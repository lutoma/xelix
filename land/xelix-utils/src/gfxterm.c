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
#include <sys/termios.h>
#include <sys/param.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "util.h"
#include "tmt.h"

#define BLOCK_HEIGHT 15
#define BLOCK_WIDTH 8
#define BLOCK_TEXTOFFSET 3

#define block_ptr(row, col) (&fb_addr[row * BLOCK_HEIGHT * (fb_pitch / 4) + \
	 col * BLOCK_WIDTH])

TMT* vt;
FT_Face face;
FT_Face face_bold;
uint32_t* fb_addr = NULL;
uint32_t fb_bpp = 0;
uint32_t fb_width = 0;
uint32_t fb_height = 0;
uint32_t fb_pitch = 0;
uint32_t fb_size = 0;
int ptm_fd;
int fb_fd;
int kbd_fd;
int cursor_row = 0;
int cursor_col = 0;
uint8_t* glyph_cache[256];
uint8_t* glyph_cache_bold[256];
FT_Library ft_library;

static void deallocate(bool release_fb) {
	close(kbd_fd);
	close(ptm_fd);
	tmt_close(vt);
	FT_Done_FreeType(ft_library);

	if(release_fb) {
		ioctl(fb_fd, 0x2f04, (uint32_t)0);
	}

	close(fb_fd);
}

void do_exit(int signum) {
	deallocate(true);
	exit(0);
}

uint8_t* render_glyph(char chr, int bold) {
	uint8_t** cache = bold ? glyph_cache_bold : glyph_cache;
	if(cache[chr]) {
		return cache[chr];
	}

	FT_Face cface = bold ? face_bold : face;
	if(FT_Load_Char(cface, chr, FT_LOAD_RENDER)) {
		fprintf(stderr, "FT_Load_Char error\n");
		return NULL;
	}

	FT_GlyphSlot slot = cface->glyph;
	uint8_t* glyph = calloc(1, BLOCK_HEIGHT * BLOCK_WIDTH);

	for (int y = 0; y < MIN(slot->bitmap.rows, BLOCK_HEIGHT); y++) {
		for (int x = 0; x < MIN(slot->bitmap.width, BLOCK_WIDTH); x++) {
			int y_offset = y + BLOCK_HEIGHT - BLOCK_TEXTOFFSET - slot->bitmap_top;
			int x_offset = x + slot->bitmap_left;

			uint8_t r = slot->bitmap.buffer[y * slot->bitmap.pitch + x];
			*(glyph + y_offset * BLOCK_WIDTH + x_offset) = r;
		}
	}

	cache[chr] = glyph;
	return glyph;
}

static inline uint32_t convert_color(int color, int bg) {
	if(color == -1 || color >= 9) {
		color = 0;
	}

	// RGB colors: Default, Black, red, green, yellow, blue, magenta, cyan, white, max
	const uint32_t colors_fg[] = {0xffffff, 0x646464, 0xff453a, 0x32d74b, 0xffd60a,
		0x0a84ff, 0xbf5af2, 0x5ac8fa, 0xffffff, 0xffffff};
	const uint32_t colors_bg[] = {0x1e1e1e, 0x1e1e1e, 0xff453a, 0x32d74b, 0xffd60a,
		0x0a84ff, 0xbf5af2, 0x5ac8fa, 0xffffff, 0xffffff};

	return (bg ? colors_bg : colors_fg)[color];
}

void write_char(const TMTSCREEN* screen, int row, int col) {
	TMTCHAR chr = screen->lines[row]->chars[col];
	uint32_t fg_col = convert_color(chr.a.fg, 0);
	uint32_t bg_col = convert_color(chr.a.bg, 1);

	for(int i = 0; i < BLOCK_HEIGHT; i++) {
		memset32(block_ptr(row, col) + i * fb_pitch/4, bg_col, BLOCK_WIDTH);
	}

	if(!chr.c || chr.c == ' ') {
		return;
	}

	uint8_t* glyph = render_glyph(chr.c, chr.a.bold);
	if(!glyph) {
		fprintf(stderr, "render_glyph %d error\n", chr.c);
		return;
	}

	uint32_t* addr = block_ptr(row, col);

	for (int y = 0; y < BLOCK_HEIGHT; y++) {
		for (int x = 0; x < BLOCK_WIDTH; x++) {
			// FIXME
			unsigned char fg[4] = {fg_col >> 16, fg_col >> 8, fg_col, glyph[y * BLOCK_WIDTH + x]};
			unsigned char bg[3] = {bg_col >> 16, bg_col >> 8, bg_col};

			uint32_t alpha = fg[3] + 1;
			uint32_t inv_alpha = 256 - fg[3];

			*(addr + y * fb_pitch/4 + x) = ((alpha * fg[0] + inv_alpha * bg[0]) >> 8) << 16 |
				((alpha * fg[1] + inv_alpha * bg[1]) >> 8) << 8 |
				((alpha * fg[2] + inv_alpha * bg[2]) >> 8);

		}
	}
}

void tmt_callback(tmt_msg_t m, TMT *vt, const void *a, void *p) {
	const TMTSCREEN *s = tmt_screen(vt);
	const TMTPOINT *c = tmt_cursor(vt);

	switch (m){
		case TMT_MSG_BELL:
			/* the terminal is requesting that we ring the bell/flash the
			 * screen/do whatever ^G is supposed to do; a is NULL
			 */
			break;

		case TMT_MSG_UPDATE:
			for (size_t r = 0; r < s->nline; r++){
				if (s->lines[r]->dirty){
					for (size_t c = 0; c < s->ncol; c++){
						write_char(s, r, c);
					}
				}
			}
			tmt_clean(vt);
			break;
		case TMT_MSG_ANSWER:
			write(ptm_fd, a, strlen(a));
			break;
		case TMT_MSG_MOVED:
			// draw new cursor, redraw previous cursor position
			for(int i = 0; i < BLOCK_HEIGHT; i++) {
				*(block_ptr(c->r, c->c) + (i * fb_pitch/4)) = 0xfd971f;
			}

			write_char(s, cursor_row, cursor_col);
			cursor_row = c->r;
			cursor_col = c->c;
			break;
	}
}

static inline void launch_child() {
	int pts_fd;

	if(openpty(&ptm_fd, &pts_fd, NULL, NULL, NULL) < 0) {
		perror("openpty");
		exit(EXIT_FAILURE);
	}

	int flags = fcntl(ptm_fd, F_GETFL);
	fcntl(ptm_fd, F_SETFL, flags | O_NONBLOCK);

	int pid = fork();
	if(pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if(pid) {
		close(pts_fd);
	} else {
		// Close all our open files and free memory
		deallocate(false);

		// Map stdin/out to pts
		close(1);
		close(2);
		close(0);
		dup2(pts_fd, 1);
		dup2(pts_fd, 2);
		dup2(pts_fd, 0);

		char* login_argv[] = { "login", NULL };
		char* login_env[] = { "TERM=ansi", NULL };
		execve("/usr/bin/login", login_argv, login_env);
	}
}

void main_loop(TMT* vt, int kbd_fd) {
	struct pollfd pfds[2];
	pfds[0].fd = kbd_fd;
	pfds[0].events = POLLIN;
	pfds[1].fd = ptm_fd;
	pfds[1].events = POLLIN;

	char* input = malloc(0x1000);
	while(1) {
		pfds[0].revents = 0;
		pfds[1].revents = 0;

		if(poll(pfds, 2, -1) < 1) {
			continue;
		}

		if(pfds[0].revents & POLLIN) {
			int nread = read(kbd_fd, input, 0x1000);
			if(nread) {
				write(ptm_fd, input, nread);
			}
		}

		if(pfds[1].revents & POLLIN) {
			int nread = read(ptm_fd, input, 0x1000);
			if(nread) {
				tmt_write(vt, input, nread);
			}
		}
	}

	free(input);
}

static inline void freetype_init() {
	if(FT_Init_FreeType(&ft_library)) {
		fprintf(stderr, "Could not initialize freetype library\n");
		exit(EXIT_FAILURE);
	}

	if(FT_New_Face(ft_library, "/usr/share/fonts/DejaVuSansMono.ttf", 0, &face)) {
		fprintf(stderr, "Could not read font\n");
		exit(EXIT_FAILURE);
	}

	if(FT_Set_Char_Size(face, 600, 0, 100, 0 )) {
		fprintf(stderr, "Could not set char size\n");
		exit(EXIT_FAILURE);
	}

	if(FT_New_Face(ft_library, "/usr/share/fonts/DejaVuSansMono-Bold.ttf", 0, &face_bold)) {
		fprintf(stderr, "Could not read font\n");
		exit(EXIT_FAILURE);
	}

	if(FT_Set_Char_Size(face_bold, 600, 0, 100, 0 )) {
		fprintf(stderr, "Could not set char size\n");
		exit(EXIT_FAILURE);
	}
}

int main() {
	struct sigaction act = {
		.sa_handler = do_exit
	};

	if(sigaction(SIGCHLD, &act, NULL) < 0) {
		perror("Could not set signal handler");
		exit(EXIT_FAILURE);
	}

	// initialize freetype, prerender frequent characters
	freetype_init();
	for(char c = 1; c <= '~'; c++) {
		render_glyph(c, 0);
		render_glyph(c, 1);
	}

	// Get framebuffer
	fb_fd = open("/dev/gfx1", O_WRONLY);
	if(!fb_fd) {
		perror("Could not open /dev/gfx1");
		exit(EXIT_FAILURE);
	}

	fb_bpp = ioctl(fb_fd, 0x2f02, (uint32_t)0);
	fb_width = ioctl(fb_fd, 0x2f05, (uint32_t)0);
	fb_height = ioctl(fb_fd, 0x2f06, (uint32_t)0);
	fb_pitch = ioctl(fb_fd, 0x2f07, (uint32_t)0);
	fb_size = fb_height * fb_pitch;

	// Get framebuffer address
	fb_addr = (uint32_t*)ioctl(fb_fd, 0x2f01, (uint32_t)0);
	if(!fb_addr || !fb_size) {
		perror("Could not get memory mapping");
		exit(EXIT_FAILURE);
	}

	// Map framebuffer into our address space and clear it
	ioctl(fb_fd, 0x2f03, (uint32_t)0);
	memset32(fb_addr, 0x1e1e1e, fb_size / 4);

	// Open keyboard device
	kbd_fd = open("/dev/keyboard1", O_RDONLY | O_NONBLOCK);
	if(!kbd_fd) {
		perror("Could not open /dev/keyboard1");
		exit(EXIT_FAILURE);
	}

	int rows = (fb_height / BLOCK_HEIGHT) - 1;
	int cols = fb_width / BLOCK_WIDTH;
	vt = tmt_open(rows, cols, tmt_callback, NULL, NULL);
	if(!vt) {
		fprintf(stderr, "Could not open TMT virtual terminal.\n");
		exit(EXIT_FAILURE);
	}

	launch_child();
	main_loop(vt, kbd_fd);
}
