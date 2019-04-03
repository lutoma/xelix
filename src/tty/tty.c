/* tty.c: Terminal emulation
 * Copyright © 2019 Lukas Martini
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tty/tty.h>
#include <tty/fbtext.h>
#include <tty/keyboard.h>
#include <tty/ecma48.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <mem/kmalloc.h>
#include <panic.h>
#include <errno.h>
#include <stdlib.h>
#include <log.h>

#define SCROLLBACK_PAGES 15

static inline void handle_nonprintable(char chr) {
	if(chr == term->termios.c_cc[VEOL]) {
		term->cur_row++;
		term->cur_col = 0;

		if(term->cur_row >= term->drv->rows) {
			term->drv->scroll_line();
			term->cur_row--;
		}
		return;
	}

	if(chr == term->termios.c_cc[VERASE] || chr == 0x7f) {
		//term->scrollback_end--;
		// FIXME limit checking
		term->drv->write(--term->cur_col, term->cur_row, ' ', false, term->bg_color, term->bg_color);
		return;
	}

	if(chr == '\t') {
		term->cur_col += 8 - (term->cur_col % 8);
		return;
	}

	if(chr == term->termios.c_cc[VINTR]) {
		tty_write("^C\n", 3);
	}
}

size_t tty_write(char* source, size_t size) {
	bool remove_cursor = false;

	if(!term->drv) {
		return 0;
	}

	for(int i = 0; i < size; i++) {
		char chr = source[i];

		if(chr == 033) {
			size_t skip = tty_handle_escape_seq(source + i, size - i);
			if(skip) {
				i += skip;
				continue;
			}
		}

		if(term->cur_col >= term->drv->cols) {
			serial_printf("width overflow.\n");
			remove_cursor = true;
			handle_nonprintable(term->termios.c_cc[VEOL]);
			i--;
			continue;
		}

		if(chr > 31 && chr < 127) {
			term->last_char = chr;
			term->drv->write(term->cur_col, term->cur_row, chr,
				term->write_bdc, term->fg_color, term->bg_color);
			term->cur_col++;
		} else {
			remove_cursor = true;
			handle_nonprintable(chr);
		}
	}

	term->drv->set_cursor(term->cur_col, term->cur_row, remove_cursor);
	return size;
}

/* Sysfs callbacks */
static size_t sfs_write(void* source, size_t size, size_t offset, void* meta) {
	return tty_write((char*)source, size);
}
static size_t sfs_read(void* dest, size_t size, size_t offset, void* meta) {
	return tty_read((char*)dest, size);
}

void tty_init() {
	term = zmalloc(sizeof(struct terminal));
	term->fg_color = FG_COLOR_DEFAULT;
	term->bg_color = BG_COLOR_DEFAULT;

	term->termios.c_lflag = ECHO | ICANON | ISIG;
	term->termios.c_cc[VEOF] = 4;
	term->termios.c_cc[VEOL] = '\n';
	term->termios.c_cc[VERASE] = 8;
	term->termios.c_cc[VINTR] = 3;
	term->termios.c_cc[VKILL] = 21;
	//term->termios.c_cc[VMIN] = 'minimum';
	term->termios.c_cc[VQUIT] = 28;
	term->termios.c_cc[VSTART] = 17;
	term->termios.c_cc[VSTOP] = 19;
	term->termios.c_cc[VSUSP] = 26;
	//term->termios.c_cc[VTIME] = 'Timeout';

	tty_keyboard_init();
	term->drv = tty_fbtext_init();
	if(!term->drv) {
		panic("Could not initialize tty driver");
	}

	term->scrollback_size = term->drv->cols * term->drv->rows * SCROLLBACK_PAGES;
	term->scrollback = zmalloc(term->scrollback_size);
	term->scrollback_end = -1;

	log(LOG_DEBUG, "tty: Can render %d columns, %d rows\n", term->drv->cols, term->drv->rows);
	sysfs_add_dev("stdin", sfs_read, NULL);
	sysfs_add_dev("stdout", NULL, sfs_write);
	sysfs_add_dev("stderr", NULL, sfs_write);
}
