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
#include <mem/kmalloc.h>
#include <panic.h>
#include <errno.h>
#include <stdlib.h>
#include <log.h>

#define FG_COLOR_DEFAULT 0xe9e9e9
#define BG_COLOR_DEFAULT 0x000000

#define TIOCGWINSZ   0x400E
#define SCROLLBACK_PAGES 15

struct winsize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
};

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

static uint32_t fg_col = FG_COLOR_DEFAULT;
static uint32_t bg_col = BG_COLOR_DEFAULT;

static inline void put_char(char chr) {
	if(chr == '\n' || cur_col >= drv->cols) {
		cur_row++;
		cur_col = 0;

		if(cur_row >= drv->rows) {
			drv->scroll_line();
			cur_row--;
		}

		if(chr == '\n') {
			return;
		}
	}

	if(chr == '\t') {
		cur_col += 8 - (cur_col % 8);
		return;
	}

	drv->write(cur_col, cur_row, chr, fg_col, bg_col);
	cur_col++;
}

#define ESC_ADVANCE(x) { cur_str += x; spos += x; }
static size_t handle_escape_seq(char* str, size_t str_len) {
	size_t spos = 0;

	// Check if string contains enough space for an escape sequence
	if(str_len < 3) {
		return 0;
	}

	char* cur_str = str;
	ESC_ADVANCE(1);

	if(*cur_str != '[') {
		return 0;
	}
	ESC_ADVANCE(1);

	// An escape code may have a number of intermediate bytes
	int intermediate_start = 0;
	while(spos <= str_len && (*cur_str == ';' || ('0' <= *cur_str && '9' >= *cur_str))) {
		if(!intermediate_start) {
			intermediate_start = spos;
		}
		ESC_ADVANCE(1);
	}
	if(spos == str_len) {
		return 0;
	}

	char* intermediate = NULL;
	if(intermediate_start) {
		intermediate = strndup(str + intermediate_start, spos - intermediate_start);
	}

	int arg;
	switch(*cur_str) {
		case 'm':
			if(!intermediate) {
				return 0;
			}
			if(!strcmp(intermediate, "0")) {
				fg_col = FG_COLOR_DEFAULT;
				bg_col = BG_COLOR_DEFAULT;
				break;
			}

			int color = 0;
			if(!strncmp(intermediate, "01;", 3)) {
				color = atoi(intermediate + 3);
			} else {
				color = atoi(intermediate);
			}

			switch(color) {
				case 30: // black
					fg_col = BG_COLOR_DEFAULT; break;
				case 31: // red
					fg_col = 0xF92672; break;
				case 32: // green
					fg_col = 0xA6E22E; break;
				case 33: // yellow
					fg_col = 0xE6DB74; break;
				case 34: // blue
					fg_col = 0x66D9EF; break;
				case 35: // magenta
					fg_col = 0xFD5FF0; break;
				case 36: // cyan
					fg_col = 0xA1EFE4; break;
				case 37: // white
					fg_col = FG_COLOR_DEFAULT; break;
				default:
					fg_col = FG_COLOR_DEFAULT;
			}

			break;
		case 'J':
			arg = 0;
			if(intermediate) {
				arg = atoi(intermediate);
			}

			switch(arg) {
				case 0:
					drv->clear(cur_col, cur_row, drv->cols, drv->rows); break;
				case 1:
					drv->clear(0, 0, cur_col, cur_row); break;
				case 2:
					drv->clear(0, 0, drv->cols, drv->rows); break;
			}
			break;
		case 'H':
			if(!intermediate) {
				cur_col = 0;
				cur_row = 0;
			} else {
				printf("rip rup\n");
			}
			break;
		default:
			spos = 0;
			break;
	}

	if(intermediate) {
		kfree(intermediate);
	}

	return spos;
}

size_t tty_write(char* source, size_t size) {
	if(!drv) {
		return 0;
	}

	for(int i = 0; i < size; i++) {
		char chr = source[i];

		// FIXME Should just memcopy whole string
		//*(scrollback + (++scrollback_end % scrollback_size)) = chr;

		if(chr == 033) {
			size_t skip = handle_escape_seq(source + i, size - i);
			if(skip) {
				i += skip;
				continue;
			}
		}

		put_char(chr);
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

int tty_ioctl(const char* path, int request, void* arg) {
	if(request != TIOCGWINSZ) {
		sc_errno = ENOSYS;
		return -1;
	}

	if(!arg) {
		sc_errno = EINVAL;
		return -1;
	}

	struct winsize* ws = (struct winsize*)arg;
	ws->ws_row = drv->rows;
	ws->ws_col = drv->cols;
	ws->ws_xpixel = drv->xpixel;
	ws->ws_ypixel = drv->ypixel;
	return 0;
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
	if(!read_buf || read_len >= read_buf_size) {
		return;
	}

	char c = keycode_to_char(code, code2);
	if(!c) {
		return;
	}

	// Backspace
	if(c == 0x8 || c == 0x7f) {
		if(read_len) {
			read_len--;
			scrollback_end--;
			drv->write(--cur_col, cur_row, ' ', bg_col, bg_col);
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
