/* tty.c: tty initialization
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
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <memory/kmalloc.h>
#include <panic.h>
#include <log.h>

#define SCROLLBACK_PAGES 15

static struct tty_driver* drv;

static size_t scrollback_size;
static size_t scrollback_end;
static char* scrollback;

static uint32_t cur_col = 0;
static uint32_t cur_row = 0;

static char* read_buf;
static size_t read_buf_size;
static size_t read_len;
static bool read_done = false;

static inline void put_char(char chr) {
	*(scrollback + (++scrollback_end % scrollback_size)) = chr;

	if(chr == '\n' || cur_col > drv->cols) {
		cur_row++;
		cur_col = 0;

		if(cur_row >= drv->rows) {
			drv->scroll_line();
			cur_row--;
		}
		return;
	}

	// FIXME
	if(chr == '\t') {
		cur_col += cur_col % 4 ? cur_col % 4 : 4;
		return;
	}

	drv->write(cur_col, cur_row, chr);
	cur_col++;
}

size_t tty_write(char* source, size_t size) {
	if(!drv) {
		return 0;
	}

	for(int i = 0; i < size; i++) {
		put_char(source[i]);
	}
	return size;
}

size_t tty_read(char* dest, size_t size) {
	read_len = 0;
	read_buf = dest;
	read_buf_size = size;

	while(!read_done) {
		asm("hlt");
	}

	read_buf = NULL;
	read_buf_size = 0;
	read_done = false;
	return read_len;
}

static char keycode_to_char(uint8_t code, uint8_t code2) {
	static bool shift = false;

	if(!read_buf || read_len >= read_buf_size) {
		return 0;
	}

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
	char c = keycode_to_char(code, code2);
	if(!c) {
		return;
	}

	// Backspace
	if(c == 0x8 || c == 0x7f) {
		if(read_len) {
			read_len--;
			scrollback_end--;
			drv->write(--cur_col, cur_row, ' ');
		}
		return;
	}

	read_buf[read_len++] = c;
	put_char(c);

	if(c == '\n' || read_len >= read_buf_size) {
		read_done = true;
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
	tty_keyboard_init();
	drv = tty_fbtext_init();
	if(!drv) {
		panic("Could not initialize tty driver");
	}

	scrollback_size = drv->cols * drv->rows * SCROLLBACK_PAGES;
	scrollback = zmalloc(scrollback_size);
	scrollback_end = -1;

	log(LOG_DEBUG, "tty: Can render %d columns, %d rows\n", drv->cols, drv->rows);
	sysfs_add_dev("stdin", sfs_read, NULL, NULL);
	sysfs_add_dev("stdout", NULL, sfs_write, NULL);
	sysfs_add_dev("stderr", NULL, sfs_write, NULL);
}
