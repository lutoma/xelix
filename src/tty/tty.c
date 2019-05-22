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
#include <tty/input.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <mem/kmalloc.h>
#include <panic.h>
#include <errno.h>
#include <stdlib.h>
#include <log.h>

#define SCROLLBACK_PAGES 15

uint8_t default_c_cc[NCCS] = {
	0,
	4, // VEOF
	'\n', // VEOL
	8, // VERASE
	3, // VINTR
	21, // VKILL
	0,  // VMIN
	28, // VQUIT
	17, // VSTART
	19, // VSTOP
	26, // VSUSP
	0,
};

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
	if(!term || !term->drv) {
		return 0;
	}

	bool remove_cursor = false;
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
static size_t sfs_write(struct vfs_file* fp, void* source, size_t size, struct task* rtask) {
	return tty_write((char*)source, size);
}
static size_t sfs_read(struct vfs_file* fp, void* dest, size_t size, struct task* rtask) {
	return tty_read((char*)dest, size);
}

void tty_init() {
	term = zmalloc(sizeof(struct terminal));
	term->fg_color = FG_COLOR_DEFAULT;
	term->bg_color = BG_COLOR_DEFAULT;

	term->termios.c_lflag = ECHO | ICANON | ISIG;
	memcpy(term->termios.c_cc, default_c_cc, sizeof(default_c_cc));

	tty_keyboard_init();
	term->drv = tty_fbtext_init();
	if(!term->drv) {
		panic("Could not initialize tty driver");
	}

	term->scrollback_size = term->drv->cols * term->drv->rows * SCROLLBACK_PAGES;
	term->scrollback = zmalloc(term->scrollback_size);
	term->scrollback_end = -1;
	log(LOG_DEBUG, "tty: Can render %d columns, %d rows\n", term->drv->cols, term->drv->rows);

	struct vfs_callbacks stdin_cb = {
		.read = sfs_read,
		.ioctl = tty_ioctl,
		.poll = tty_poll,
	};
	struct vfs_callbacks stdout_cb = {
		.write = sfs_write,
		.ioctl = tty_ioctl,
	};

	sysfs_add_dev("stdin", &stdin_cb);
	sysfs_add_dev("stdout", &stdout_cb);
	sysfs_add_dev("stderr", &stdout_cb);
}
