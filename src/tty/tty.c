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
#include <tty/keymap.h>
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
		term->drv->write(--term->cur_col, term->cur_row, ' ', term->bg_color, term->bg_color);
		return;
	}

	if(chr == '\t') {
		term->cur_col += 8 - (term->cur_col % 8);
		return;
	}
}

static inline void put_char(char chr) {
	if(chr > 31 && chr < 127) {
		term->drv->write(term->cur_col, term->cur_row, chr, term->fg_color, term->bg_color);
		term->cur_col++;
	} else {
		handle_nonprintable(chr);
	}
}

size_t tty_write(char* source, size_t size) {
	if(!term->drv) {
		return 0;
	}

	for(int i = 0; i < size; i++) {
		char chr = source[i];

		// FIXME Should just memcopy whole string
		//*(scrollback + (++scrollback_end % scrollback_size)) = chr;

		if(chr == 033) {
			size_t skip = tty_handle_escape_seq(source + i, size - i);
			if(skip) {
				i += skip;
				continue;
			}
		}

		if(term->cur_col >= term->drv->cols) {
			put_char(term->termios.c_cc[VEOL]);
		}
		put_char(chr);
	}
	return size;
}

size_t tty_read(char* dest, size_t size) {
	term->read_len = 0;
	term->read_buf = dest;
	term->read_buf_size = size;

	while(!term->read_done) {
		asm("hlt");
	}

	term->read_buf = NULL;
	term->read_buf_size = 0;
	term->read_done = false;
	return term->read_len;
}

static char keycode_to_char(uint8_t code, uint8_t code2) {
	static bool shift = false;

	switch(code) {
		case 0x2a:
		case 0x36:
			shift = true;
			return 0;
		case 0xaa:
		case 0xb6:
			shift = false;
			return 0;
	}

	if(code == 0x2a || code == 0x36) {
		shift = true;
		return 0;
	}

	if(code == 0xaa || code == 0xb6) {
		shift = false;
		return 0;
	}

	uint32_t rcode = code;
	if(shift) {
		rcode += 256;
	}
	if(rcode > 512) {
		return 0;
	}

	return tty_keymap_en[rcode];
}


// Input callback – Called by keyboard.c
void tty_input_cb(uint8_t code, uint8_t code2) {
	if(!term || !term->read_buf || term->read_len >= term->read_buf_size) {
		return;
	}

	char c = keycode_to_char(code, code2);
	if(!c) {
		return;
	}

	switch(c) {
		case 0x8:
		case 0x7f:
			c = term->termios.c_cc[VERASE]; break;
		case '\n':
			c = term->termios.c_cc[VEOL]; break;
	}

	if(c == term->termios.c_cc[VERASE] && term->termios.c_lflag & ICANON) {
		if(!term->read_len) {
			return;
		}

		term->read_len--;
	} else {
		term->read_buf[term->read_len++] = c;
	}

	if(term->termios.c_lflag & ECHO) {
		put_char(c);
	}

	if(c == term->termios.c_cc[VEOL] || term->read_len >= term->read_buf_size) {
		term->read_done = true;
	}
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

	term->termios.c_lflag = ECHO | ICANON;
	term->termios.c_cc[VEOF] = 0;
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
	sysfs_add_dev("stdin", sfs_read, NULL, NULL);
	sysfs_add_dev("stdout", NULL, sfs_write, NULL);
	sysfs_add_dev("stderr", NULL, sfs_write, NULL);
}
