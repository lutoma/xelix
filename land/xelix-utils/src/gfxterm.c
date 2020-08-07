/* Copyright © 2020 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#include <locale.h>
#include <sys/termios.h>
#include <sys/param.h>
#include <wchar.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "util.h"
#define TMT_HAS_WCWIDTH
#include "tmt.h"

#define BLOCK_HEIGHT 15
#define BLOCK_WIDTH 8
#define BLOCK_TEXTOFFSET 3

#define block_ptr(row, col) (&fb.addr[row * BLOCK_HEIGHT * (fb.pitch / 4) + \
	 col * BLOCK_WIDTH])

struct msg_window_new {
	void* addr;
	char title[1024];
	size_t width;
	size_t height;
	int32_t x;
	int32_t y;
};

struct msg_blit {
	uint32_t wid;
	size_t width;
	size_t height;
	int32_t x;
	int32_t y;
};

struct {
	int fd;
	uint32_t* addr;
	uint32_t bpp;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t size;
} fb;

TMT* vt;
FT_Face face;
FT_Face face_bold;
int ptm_fd;
int kbd_fd;
int cursor_row = 0;
int cursor_col = 0;
FILE* serial;

int bg_alpha = 0xb8;

#define CACHE_MAX 256
uint8_t* glyph_cache[CACHE_MAX];
uint8_t* glyph_cache_bold[CACHE_MAX];
FT_Library ft_library;

static void deallocate(bool release_fb) {
	close(kbd_fd);
	close(ptm_fd);
	tmt_close(vt);
	FT_Done_FreeType(ft_library);
/*
	if(release_fb) {
		ioctl(fb.fd, 0x2f04, (uint32_t)0);
	}
*/
	close(fb.fd);
}

void do_exit(int signum) {
	deallocate(true);
	exit(0);
}

uint8_t* render_glyph(wchar_t chr, int bold) {
	uint8_t** cache = bold ? glyph_cache_bold : glyph_cache;
	if(chr < CACHE_MAX && cache[chr]) {
		return cache[chr];
	}

	FT_Face cface = bold ? face_bold : face;
	if(FT_Load_Char(cface, chr, FT_LOAD_RENDER)) {
		return NULL;
	}

	FT_GlyphSlot slot = cface->glyph;
	uint8_t* glyph = calloc(1, BLOCK_HEIGHT * BLOCK_WIDTH);

	for (int y = 0; y < slot->bitmap.rows; y++) {
		int y_offset = y + BLOCK_HEIGHT - BLOCK_TEXTOFFSET - slot->bitmap_top;
		if(y_offset < 0 || y_offset >= BLOCK_HEIGHT) {
			continue;
		}

		for (int x = 0; x < slot->bitmap.width; x++) {
			int x_offset = x + slot->bitmap_left;
			if(x_offset >= BLOCK_WIDTH) {
				break;
			}

			uint8_t r = slot->bitmap.buffer[y * slot->bitmap.pitch + x];
			*(glyph + y_offset * BLOCK_WIDTH + x_offset) = r;
		}
	}

	if(chr < CACHE_MAX) {
		cache[chr] = glyph;
	}
	return glyph;
}

static inline uint32_t convert_color(int color, int bg) {
	if(color == -1 || color >= 9) {
		color = 0;
	}

	// RGBA colors: Default, Black, red, green, yellow, blue, magenta, cyan, white, max
	const uint32_t colors_fg[] = {0xffffffff, 0xff646464, 0xffff453a, 0xff32d74b, 0xffffd60a,
		0xff0a84ff, 0xffbf5af2, 0xff5ac8fa, 0xffffffff, 0xffffffff};
	const uint32_t colors_bg[] = {0xff1e1e1e, 0xff1e1e1e, 0xffff453a, 0xff32d74b, 0xffffd60a,
		0xff0a84ff, 0xffbf5af2, 0xff5ac8fa, 0xffffffff, 0xffffffff};

	return (bg ? colors_bg : colors_fg)[color];
}

static inline uint32_t rgba_interp(uint32_t src, uint32_t dst, uint32_t t) {
    const uint32_t s = 255 - t;
    return (
        (((((src >> 0)  & 0xff) * s +
           ((dst >> 0)  & 0xff) * t) >> 8)) |
        (((((src >> 8)  & 0xff) * s +
           ((dst >> 8)  & 0xff) * t)     )  & ~0xff) |
        (((((src >> 16) & 0xff) * s +
           ((dst >> 16) & 0xff) * t) << 8)  & ~0xffff) |
        (((((src >> 24) & 0xff) * s +
           ((dst >> 24) & 0xff) * t) << 16) & ~0xffffff)
    );
}

void write_char(const TMTSCREEN* screen, int row, int col) {
	TMTCHAR chr = screen->lines[row]->chars[col];
	uint32_t fg_col = convert_color(chr.a.fg, 0);
	uint32_t bg_col = convert_color(chr.a.bg, 1);

	for(int i = 0; i < BLOCK_HEIGHT; i++) {
		memset32(block_ptr(row, col) + i * fb.pitch/4, bg_col, BLOCK_WIDTH);
	}

	uint8_t* glyph = render_glyph(chr.c, chr.a.bold);
	if(!glyph) {
		fprintf(stderr, "render_glyph %d error\n", chr.c);
		return;
	}

	uint32_t* addr = block_ptr(row, col);
	for (int y = 0; y < BLOCK_HEIGHT; y++) {
		for (int x = 0; x < BLOCK_WIDTH; x++) {

			int alpha = 0xff;

			/* Rescale glyph alpha from [0 -> 0xff] to [bg_alpha - 0xff] so
			 * compositor renders background with transparency - but only for
			 * the default color
			 */
			if(chr.a.bg < 2) {
				double slope = 1.0 * (0xff - bg_alpha) / 0xff;
				alpha = bg_alpha + slope * glyph[y * BLOCK_WIDTH + x];
			}

			*(addr + y * fb.pitch/4 + x) = rgba_interp(bg_col, fg_col, glyph[y * BLOCK_WIDTH + x]) & 0xffffff | alpha << 24;
		}
	}
}

static inline void send_blit_msg(size_t x, size_t y, size_t width, size_t height) {
	uint16_t two = 2;
	struct msg_blit msg = {
		.wid = 0,
		.width = width,
		.height = height,
		.x = x,
		.y = y
	};

	write(fb.fd, &two, 2);
	write(fb.fd, &msg, sizeof(struct msg_blit));
}

void tmt_callback(tmt_msg_t m, TMT *vt, const void *a, void *p) {
	const TMTSCREEN *s = tmt_screen(vt);
	const TMTPOINT *c = tmt_cursor(vt);

	size_t update_start = -1;
	size_t update_end = 0;

	switch (m){
		case TMT_MSG_BELL:
			/* the terminal is requesting that we ring the bell/flash the
			 * screen/do whatever ^G is supposed to do; a is NULL
			 */
			break;

		case TMT_MSG_UPDATE:
			for (size_t r = 0; r < s->nline; r++){
				if (s->lines[r]->dirty){
					if(update_start == -1) {
						update_start = r * BLOCK_HEIGHT;
					}

					update_end = (r+1) * BLOCK_HEIGHT;

					for (size_t c = 0; c < s->ncol; c++){
						write_char(s, r, c);
					}
				}
			}
			tmt_clean(vt);
			send_blit_msg(0, update_start, fb.width, update_end - update_start);
			break;
		case TMT_MSG_ANSWER:
			write(ptm_fd, a, strlen(a));
			break;
		case TMT_MSG_MOVED:
			// draw new cursor, redraw previous cursor position
			for(int i = 0; i < BLOCK_HEIGHT; i++) {
				*(block_ptr(c->r, c->c) + (i * fb.pitch/4)) = 0xffffffff;
			}

			write_char(s, cursor_row, cursor_col);
			send_blit_msg(cursor_col * BLOCK_WIDTH, cursor_row * BLOCK_HEIGHT, 1, BLOCK_HEIGHT);

			cursor_row = c->r;
			cursor_col = c->c;
			send_blit_msg(cursor_col * BLOCK_WIDTH, cursor_row * BLOCK_HEIGHT, 1, BLOCK_HEIGHT);
			break;
	}
}

static inline void load_font(const char* path, FT_Face* face) {
	if(FT_New_Face(ft_library, path, 0, face)) {
		perror("Could not read font");
		exit(EXIT_FAILURE);
	}

	if(FT_Set_Char_Size(*face, 600, 0, 100, 0 )) {
		perror("Could not set char size");
		exit(EXIT_FAILURE);
	}
}

void main_loop(TMT* vt, int kbd_fd) {
	struct pollfd pfds[2];
	pfds[0].fd = kbd_fd;
	pfds[0].events = POLLIN;
	pfds[1].fd = ptm_fd;
	pfds[1].events = POLLIN;

	size_t buf_size = getpagesize();
	char* input = malloc(buf_size);

	while(1) {
		pfds[0].revents = 0;
		pfds[1].revents = 0;

		if(poll(pfds, 2, -1) < 1) {
			continue;
		}

		if(pfds[0].revents & POLLIN) {
			size_t nread = read(kbd_fd, input, buf_size);
			if(nread) {
				write(ptm_fd, input, nread);
			}
		}

		if(pfds[1].revents & POLLIN) {
			size_t nread = read(ptm_fd, input, buf_size);
			if(nread) {
				tmt_write(vt, input, nread);
			}
		}
	}

	free(input);
}

int main() {
	serial = fopen("/dev/serial1", "w");
	setvbuf(serial, NULL, _IONBF, 0);

	struct sigaction act = {
		.sa_handler = do_exit
	};
	if(sigaction(SIGCHLD, &act, NULL) < 0) {
		perror("Could not set signal handler");
	}

	if(!setlocale(LC_ALL, "en_US.UTF-8")) {
		perror("Could not set locale");
	}

	// initialize freetype, prerender frequent characters
	if(FT_Init_FreeType(&ft_library)) {
		perror("Could not initialize freetype");
		exit(EXIT_FAILURE);
	}

	load_font("/usr/share/fonts/Gintronic-Regular.otf", &face);
	load_font("/usr/share/fonts/Gintronic-Bold.otf", &face_bold);

	for(char c = 1; c <= '~'; c++) {
		render_glyph(c, 0);
		render_glyph(c, 1);
	}

	FILE* serial = fopen("/dev/serial1", "w");
	setvbuf(serial, NULL, _IONBF, 0);

	// Get gfxbus handle
	fb.fd = open("/dev/gfxbus", O_RDWR);
	if(!fb.fd) {
		perror("Could not open /dev/gfxbus");
		exit(EXIT_FAILURE);
	}

	fb.bpp = 32;
	fb.width = 800;
	fb.height = 600;
	fb.pitch = fb.width * 4;
	fb.size = fb.pitch * fb.height * 4;
	fb.addr = (uint32_t*)ioctl(fb.fd, 0x2f02, fb.size);
	memset32(fb.addr, 0x1e1e1e | bg_alpha << 24, fb.size / 4);

	struct msg_window_new msg = {
		.addr = fb.addr,
		.title = "~ : bash - Terminal",
		.width = 800,
		.height = 600,
		.x = 50,
		.y = 100
	};

	int one = 1;
	write(fb.fd, &one, 2);
	write(fb.fd, &msg, sizeof(struct msg_window_new));
	send_blit_msg(0, 0, fb.width, fb.height);

	// Open keyboard device
	kbd_fd = open("/dev/keyboard1", O_RDONLY | O_NONBLOCK);
	if(!kbd_fd) {
		perror("Could not open /dev/keyboard1");
		exit(EXIT_FAILURE);
	}

	unsigned int rows = (fb.height / BLOCK_HEIGHT) - 1;
	unsigned int cols = fb.width / BLOCK_WIDTH;

	vt = tmt_open(rows, cols, tmt_callback,
		NULL, L"→←↑↓■◆▒°±▒┘┐┌└┼⎺───⎽├┤┴┬│≤≥π≠£•");

	if(!vt) {
		fprintf(stderr, "Could not open TMT virtual terminal.\n");
		exit(EXIT_FAILURE);
	}

	// Get a pty and launch process
	struct winsize ws = {
		.ws_row = rows,
		.ws_col = cols,
		.ws_xpixel = fb.width,
		.ws_ypixel = fb.height,
	};

	pid_t pid = forkpty(&ptm_fd, NULL, NULL, &ws);
	if(pid < 0) {
		perror("Could not forkpty");
		exit(EXIT_FAILURE);
	}

	if(pid) {
		int flags = fcntl(ptm_fd, F_GETFL);
		fcntl(ptm_fd, F_SETFL, flags | O_NONBLOCK);
	} else {
		// Close all our open files and free memory
		deallocate(false);

		setenv("TERM", "ansi", 1);
		if(execl("/usr/bin/login", "login") < 0) {
			perror("Could not execute subprocess");
			exit(EXIT_FAILURE);
		}
	}

	main_loop(vt, kbd_fd);
}
