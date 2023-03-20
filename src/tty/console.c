/* console.c: Xelix text console
 * Copyright © 2019-2023 Lukas Martini
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

#include <tty/term.h>
#include <tty/console.h>
#include <gfx/fbtext.h>
#include <mem/kmalloc.h>
#include <stdlib.h>

#define ESC_ADVANCE() {		\
	cur_str++;				\
	if(++spos >= str_len) {	\
		return 0;			\
	}						\
}

static int cols = 0;
static int rows = 0;
static bool write_bdc = false;
static int cur_row = 0;
static int cur_col = 0;
static char last_char = '\0';
static uint32_t color_fg = 0xffffff;
static uint32_t color_bg = 0x000000;

static uint32_t convert_color(int color, bool bg) {
	// RGB colors: Black, red, green, yellow, blue, magenta, cyan, white, default
	const uint32_t colors_fg[] = {0x1e1e1e, 0xff453a, 0x32d74b, 0xffd60a,
		0x0a84ff, 0xbf5af2, 0x5ac8fa, 0xdedede, 0xffffff};
	const uint32_t colors_bg[] = {0x1e1e1e, 0xff453a, 0x32d74b, 0xffd60a,
		0x0a84ff, 0xbf5af2, 0x5ac8fa, 0xffffff, 0x1e1e1e};

	if(color < 0 || color >= ARRAY_SIZE(colors_fg)) {
		color = 8;
	}
	return (bg ? colors_bg : colors_fg)[color];
}

static int set_char_attrs(struct term* term, int args[], size_t num_args) {
	if(!num_args) {
		color_fg = 0xffffff;
		color_bg = 0x000000;
		return 0;
	}

	for(int i = 0; i < num_args; i++) {
		int attr = args[i];
		if(attr == 0) {
			color_fg = 0xffffff;
			color_bg = 0x000000;
		} else if(attr >= 30 && attr < 40) {
			color_fg = convert_color(attr - 30, 0);
		} else if(attr >= 40 && attr < 50) {
			color_bg = convert_color(attr - 40, 1);
		}
	}

	return 0;
}

static int clear(struct term* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 0;
	switch(arg) {
		case 0:
			fbtext_clear(0, cur_row, cols, rows); break;
		case 1:
			fbtext_clear(0, 0, cur_col, cur_row); break;
		case 2:
			fbtext_clear(0, 0, cols, rows); break;
	}
	return 0;
}

static int erase_in_line(struct term* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 0;
	switch(arg) {
		case 0:
			fbtext_clear(cur_col, cur_row, cols, cur_row); break;
		case 1:
			fbtext_clear(0, cur_row, cur_col, cur_row); break;
		case 2:
			fbtext_clear(0, cur_row, cols, cur_row); break;
	}
	return 0;
}

static int set_pos(struct term* term, int args[], size_t num_args) {
	if(num_args < 2) {
		cur_col = 0;
		cur_row = 0;
		return 0;
	}

	cur_row = args[0] - 1;
	cur_col = args[1] - 1;
	return 0;
}
static int set_row_pos(struct term* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 1;
	cur_row = --arg;
	return 0;
}

static int set_col_pos(struct term* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 1;
	cur_col = --arg;
	return 0;
}

static int clear_forward(struct term* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 1;
	fbtext_clear(cur_col, cur_row, cur_col + arg, cur_row);
	return 0;
}

static int repeat_char(struct term* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 1;
	if(last_char) {
		for(int i = 0; i < arg; i++) {
//			_tty_write(term, &last_char, 1);
		}
	}
	return 0;
}

static int reset(struct term* term, int args[], size_t num_args) {
	color_fg = 0xffffff;
	color_bg = 0x000000;
	return 0;
}

static int set_bdc(struct term* term, char arg) {
	switch(arg) {
		case '0': write_bdc = true; break;
		case 'B': write_bdc = false; break;
		default: return -1;
	}
	return 0;
}

/* This function gets called by the tty output handlers whenever they encounter
 * Esc (\033). It returns the number of handled characters. If an escape code
 * is not recognized, this returns 0 so the code is printed to the screen.
 */
size_t tty_handle_escape_seq(struct term* term, char* str, size_t str_len) {
	size_t spos = 0;
	char* cur_str = str;
	ESC_ADVANCE();

	// \033( or \033) for Block Drawing Character mode settings
	if(*cur_str == ')' || *cur_str == '(') {
		ESC_ADVANCE();
		return set_bdc(term, *cur_str) == -1 ? 0 : spos;
	} else if(*cur_str != '[') {
		return 0;
	}

	// Parse out any potential numeric intermediate arguments
	ESC_ADVANCE();
	int args[10];
	size_t num_args = 0;
	char* arg_start = NULL;

	while(1) {
		if('0' <= *cur_str && '9' >= *cur_str) {
			if(!arg_start) {
				arg_start = cur_str;
			}
		} else {
			if((*cur_str == ';' && !arg_start)) {
				return 0;
			}

			if(arg_start) {
				size_t len = cur_str - arg_start;
				if(num_args >= ARRAY_SIZE(args) || !len) {
					return 0;
				}

				char* astr = strndup(arg_start, len);
				args[num_args++] = atoi(astr);
				kfree(astr);
				arg_start = NULL;
			}

			if(*cur_str != ';' && *cur_str != '?') {
				break;
			}
		}

		ESC_ADVANCE();
	}

	int result = -1;
	switch(*cur_str) {
		case 'A': // CUU
			cur_row--;
			result = 0;
			break;
		case 'B': // CUD
			cur_row++;
			result = 0;
			break;
		case 'C': // CUF
		// FIXME
		case '@': // ICH
			cur_col++;
			result = 0;
			break;
		case 'D': // CUB
			cur_col--;
			result = 0;
			break;
		case 'm': result = set_char_attrs(term, args, num_args); break;
		case 'J': result = clear(term, args, num_args); break;
		case 'K': result = erase_in_line(term, args, num_args); break;
		case 'H': result = set_pos(term, args, num_args); break; // CUP
		case 'd': result = set_row_pos(term, args, num_args); break;
		case 'G': result = set_col_pos(term, args, num_args); break;
		case 'X': result = clear_forward(term, args, num_args); break;
		case 'l': result = reset(term, args, num_args); break;
		case 'b': result = repeat_char(term, args, num_args); break;
	}

	return result == -1 ? 0 : spos;
}


static inline void handle_nonprintable(struct term* term, char chr) {
	if(chr == term->termios.c_cc[VEOL]) {
		cur_row++;
		cur_col = 0;

		if(cur_row >= rows) {
			fbtext_scroll();
			cur_row--;
		}
		return;
	}

	if(chr == term->termios.c_cc[VERASE] || chr == 0x7f) {
		// FIXME limit checking
		fbtext_write(--cur_col, cur_row, ' ', false, color_fg, color_bg);
		return;
	}

	if(chr == '\t') {
		cur_col += 8 - (cur_col % 8);
		return;
	}

	if(chr == term->termios.c_cc[VINTR]) {
		//_tty_write(NULL, "^C\n", 3);
	}
}

static size_t term_write_cb(struct term* term, void* _source, size_t size) {
	char* source = (char*)_source;

	bool remove_cursor = false;
	for(int i = 0; i < size; i++) {
		char chr = source[i];

		if(chr == 033) {
			size_t skip = tty_handle_escape_seq(term, source + i, size - i);
			if(skip) {
				remove_cursor = true;
				i += skip;
			}
				continue;
		}

		if(cur_col >= cols) {
			remove_cursor = true;
			handle_nonprintable(term, term->termios.c_cc[VEOL]);
			i--;
			continue;
		}

		if(chr > 31 && chr < 127) {
			last_char = chr;
			fbtext_write(cur_col, cur_row, chr, write_bdc, color_fg, color_bg);
			cur_col++;
		} else {
			remove_cursor = true;
			handle_nonprintable(term, chr);
		}
	}

	fbtext_set_cursor(cur_col, cur_row, true);
	return size;
}

void tty_console_init(int _cols, int _rows) {
	cols = _cols;
	rows = _rows;
	term_console = term_new("console", term_write_cb);
	term_console->termios.c_oflag &= ~ONLCR;
	term_console->winsize.ws_row = rows;
	term_console->winsize.ws_col = cols;
}
