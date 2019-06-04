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
#include <tasks/task.h>
#include <stdbool.h>

#define FG_COLOR_DEFAULT 9
#define BG_COLOR_DEFAULT 9

struct tty_driver {
	uint32_t cols;
	uint32_t rows;
	uint32_t xpixel;
	uint32_t ypixel;
	uint32_t buf_size;
	void (*write)(struct terminal* term, uint32_t x, uint32_t y, char chr, bool bdc);
	void (*scroll_line)(struct terminal* term);
	void (*clear)(struct terminal* term, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y);
	void (*set_cursor)(struct terminal* term, uint32_t x, uint32_t y, bool restore);
	void (*rerender)(struct terminal* tty_old, struct terminal* tty_new);
};

struct terminal {
	struct tty_driver* drv;
	struct termios termios;
	task_t* fg_task;

	uint32_t cur_col;
	uint32_t cur_row;

	char read_buf[0x1000];
	size_t read_len;
	bool read_done;

	uint32_t fg_color;
	uint32_t bg_color;
	bool write_bdc;
	char last_char;

	void* drv_buf;
};

struct terminal* active_tty;
struct terminal ttys[10];

size_t tty_write(struct terminal* tty, char* source, size_t size);
size_t tty_read(struct terminal* tty, char* source, size_t size);
void tty_switch(int n);
void tty_init();
