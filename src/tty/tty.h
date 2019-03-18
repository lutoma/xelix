#pragma once

/* Copyright © 2019 Lukas Martini
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

#include <fs/vfs.h>
#include <tty/ioctl.h>
#include <stdbool.h>

#define FG_COLOR_DEFAULT 7
#define BG_COLOR_DEFAULT 0

struct tty_driver {
	uint32_t cols;
	uint32_t rows;
	uint32_t xpixel;
	uint32_t ypixel;
	void (*write)(uint32_t x, uint32_t y, char chr, uint32_t fg_col, uint32_t bg_col);
	void (*scroll_line)();
	void (*clear)(uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y);
};

struct terminal {
	struct tty_driver* drv;
	struct termios termios;

	size_t scrollback_size;
	size_t scrollback_end;
	char* scrollback;

	uint32_t cur_col;
	uint32_t cur_row;

	char read_buf[0x1000];
	size_t read_len;
	bool read_done;

	uint32_t fg_color;
	uint32_t bg_color;
};

struct terminal* term;

size_t tty_write(char* source, size_t size);
size_t tty_read(char* source, size_t size);
void tty_init();
void tty_put_char(char chr);

/* VFS callbacks */
static inline size_t tty_vfs_write(vfs_file_t* fp, void* source, size_t size) {
	return tty_write((char*)source, size);
}
static inline size_t tty_vfs_read(vfs_file_t* fp, void* dest, size_t size) {
	return tty_read((char*)dest, size);
}
