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
#include <errno.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <pty.h>
#include <sys/termios.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "tmt.h"

#define BLOCK_HEIGHT 15
#define BLOCK_WIDTH 9
#define BLOCK_TEXTOFFSET 3

#define block_ptr(row, col) (&fb_addr[row * BLOCK_HEIGHT * (fb_pitch / 4) + \
	 col * BLOCK_WIDTH])

FT_Face face;
FT_Face face_bold;
uint32_t* fb_addr = NULL;
uint32_t fb_bpp = 0;
uint32_t fb_width = 0;
uint32_t fb_height = 0;
uint32_t fb_pitch = 0;
uint32_t fb_size = 0;
int pty_master;
int pty_slave;
int cursor_row = 0;
int cursor_col = 0;
uint8_t* glyph_cache[255];
uint8_t* glyph_cache_bold[255];


static inline void *memset32(uint32_t *s, uint32_t v, size_t n) {
	long d0, d1;
	asm volatile("rep stosl"
		: "=&c" (d0), "=&D" (d1)
		: "a" (v), "1" (s), "0" (n)
		: "memory");
	return s;
}

static uint32_t convert_color(int color, int bg) {
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

static inline uint32_t blend(unsigned char fg[4], unsigned char bg[4]) {
	unsigned int alpha = fg[3] + 1;
	unsigned int inv_alpha = 256 - fg[3];
	uint32_t result = 0;
	result |= (unsigned char)((alpha * fg[0] + inv_alpha * bg[0]) >> 8) << 16;
	result |= (unsigned char)((alpha * fg[1] + inv_alpha * bg[1]) >> 8) << 8;
	result |= (unsigned char)((alpha * fg[2] + inv_alpha * bg[2]) >> 8);
	return result;
}

uint8_t* render_glyph(char chr, int bold) {
	if(chr < 0 || chr > 255) {
		return NULL;
	}

	uint8_t** cache = bold ? glyph_cache_bold : glyph_cache;
	if(cache[chr]) {
		return cache[chr];
	}

	FT_Face cface = bold ? face_bold : face;
	if(FT_Load_Char(cface, chr, FT_LOAD_RENDER)) {
		return NULL;
	}

	uint8_t* glyph = calloc(1, BLOCK_HEIGHT * BLOCK_WIDTH * 5);

	FT_GlyphSlot slot = cface->glyph;
	for (int y = 0; y < slot->bitmap.rows; y++) {
		for (int x = 0; x < slot->bitmap.width; x++) {
			int y_offset = y + BLOCK_HEIGHT - BLOCK_TEXTOFFSET - slot->bitmap_top;
			int x_offset = x + slot->bitmap_left;

			uint8_t r = slot->bitmap.buffer[y * slot->bitmap.pitch + x];
			*(glyph + y_offset * BLOCK_WIDTH + x_offset) = r;
		}
	}

	cache[chr] = glyph;
	return glyph;
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
			write(pty_master, a, strlen(a));
			break;

		case TMT_MSG_MOVED:
			// redraw cursor
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
	if(openpty(&pty_master, &pty_slave, NULL, NULL, NULL) < 0) {
		perror("openpty");
		exit(EXIT_FAILURE);
	}

	int flags = fcntl(pty_slave, F_GETFL);
	fcntl(pty_slave, F_SETFL, flags | O_NONBLOCK);

	int pid = fork();
	if(pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if(!pid) {
		close(1);
		close(2);
		close(0);
		dup2(pty_slave, 1);
		dup2(pty_slave, 2);
		dup2(pty_slave, 0);

		char* login_argv[] = { "bash", "-l", NULL };
		char* login_env[] = { "TERM=xelix", NULL };
		execve("/usr/bin/bash", login_argv, login_env);
	}
}

static inline int copy(int from, int to) {
	char buf[1024];
	int nread = read(from, buf, 1024);
	if(nread < 0 && errno != EAGAIN) {
		return -1;
	}

	if(nread > 0) {
		if(write(to, buf, nread) < 0) {
			return -1;
		}
	}
	return 0;
}

void main_loop(TMT* vt) {
	struct termios termios;
	tcgetattr(0, &termios);
	termios.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(0, TCSANOW, &termios);

	struct pollfd pfds[2];
	pfds[0].fd = 0;
	pfds[0].events = POLLIN;
	pfds[1].fd = pty_master;
	pfds[1].events = POLLIN;

	while(1) {
		if(poll(pfds, 2, -1) < 1) {
			continue;
		}

		for(int i = 0; i < 2; i++) {
			if(!(pfds[i].revents & POLLIN)) {
				continue;
			}

			pfds[i].revents = 0;
			int fd = pfds[i].fd;

			char input[0x5000];
			if(fd == pty_master) {
				int nread = read(pty_master, input, 0x5000);
				if(nread) {
					tmt_write(vt, input, nread);
				}
			} else if(fd == 0) {
				int nread = read(0, input, 0x5000);
				if(nread) {
					write(pty_master, input, nread);
					tmt_write(vt, input, nread);
				}
			}
		}
	}
}

static inline void freetype_init() {
	FT_Library library;
	if(FT_Init_FreeType( &library )) {
		fprintf(stderr, "Could not initialize freetype library\n");
		exit(EXIT_FAILURE);
	}

	if(FT_New_Face(library, "/usr/share/fonts/FiraCode-Regular.ttf", 0, &face)) {
		fprintf(stderr, "Could not read font\n");
		exit(EXIT_FAILURE);
	}

	if(FT_Set_Char_Size(face, 600, 0, 100, 0 )) {
		fprintf(stderr, "Could not set char size\n");
		exit(EXIT_FAILURE);
	}

	if(FT_New_Face(library, "/usr/share/fonts/FiraCode-Bold.ttf", 0, &face_bold)) {
		fprintf(stderr, "Could not read font\n");
		exit(EXIT_FAILURE);
	}

	if(FT_Set_Char_Size(face_bold, 600, 0, 100, 0 )) {
		fprintf(stderr, "Could not set char size\n");
		exit(EXIT_FAILURE);
	}
}

static inline void* gfx_init() {
	int fp = open("/dev/gfx1", O_WRONLY);
	if(!fp) {
		perror("Could not open /dev/gfx1");
		exit(EXIT_FAILURE);
	}

	fb_bpp = ioctl(fp, 0x2f02);
	fb_width = ioctl(fp, 0x2f05);
	fb_height = ioctl(fp, 0x2f06);
	fb_pitch = ioctl(fp, 0x2f07);
	fb_size = fb_height * fb_pitch;

	fb_addr = (uint32_t*)ioctl(fp, 0x2f01);
	if(!fb_addr || !fb_size) {
		perror("Could not get memory mapping");
		exit(EXIT_FAILURE);
	}

	ioctl(fp, 0x2f03);
	memset32(fb_addr, 0x1e1e1e, fb_size / 4);
}

int main() {
	freetype_init();
	gfx_init();

	int rows = (fb_height / BLOCK_HEIGHT) - 1;
	int cols = fb_width / BLOCK_WIDTH;

	TMT *vt = tmt_open(rows, cols, tmt_callback, NULL, NULL);
	if (!vt) {
		exit(EXIT_FAILURE);
	}

	launch_child();
	main_loop(vt);

	tmt_close(vt);
	ioctl(fp, 0x2f04);
	close(fp);
}
