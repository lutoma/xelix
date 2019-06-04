/* ecma48.c: ECMA48/ANSI escape sequence handling
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

#include <tty/ecma48.h>
#include <tty/tty.h>
#include <mem/kmalloc.h>
#include <stdlib.h>

static int set_char_attrs(struct terminal* term, int args[], size_t num_args) {
	if(!num_args) {
		term->fg_color = FG_COLOR_DEFAULT;
		term->bg_color = BG_COLOR_DEFAULT;
		return 0;
	}

	for(int i = 0; i < num_args; i++) {
		int attr = args[i];
		if(attr == 0) {
			term->fg_color = FG_COLOR_DEFAULT;
			term->bg_color = BG_COLOR_DEFAULT;
		} else if(attr >= 30 && attr < 40) {
			term->fg_color = attr - 30;
		} else if(attr >= 40 && attr < 50) {
			term->bg_color = attr - 40;
		}
	}
	return 0;
}

static int clear(struct terminal* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 0;
	switch(arg) {
		case 0:
			term->drv->clear(term, 0, term->cur_row, term->drv->cols, term->drv->rows); break;
		case 1:
			term->drv->clear(term, 0, 0, term->cur_col, term->cur_row); break;
		case 2:
			term->drv->clear(term, 0, 0, term->drv->cols, term->drv->rows); break;
	}
	return 0;
}

static int erase_in_line(struct terminal* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 0;
	switch(arg) {
		case 0:
			term->drv->clear(term, term->cur_col, term->cur_row, term->drv->cols, term->cur_row); break;
		case 1:
			term->drv->clear(term, 0, term->cur_row, term->cur_col, term->cur_row); break;
		case 2:
			term->drv->clear(term, 0, term->cur_row, term->drv->cols, term->cur_row); break;
	}
	return 0;
}

static int set_pos(struct terminal* term, int args[], size_t num_args) {
	if(num_args < 2) {
		term->cur_col = 0;
		term->cur_row = 0;
		return 0;
	}

	term->cur_row = args[0] - 1;
	term->cur_col = args[1] - 1;
	return 0;
}
static int set_row_pos(struct terminal* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 1;
	term->cur_row = --arg;
	return 0;
}

static int set_col_pos(struct terminal* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 1;
	term->cur_col = --arg;
	return 0;
}

static int clear_forward(struct terminal* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 1;
	term->drv->clear(term, term->cur_col, term->cur_row, term->cur_col + arg, term->cur_row);
	return 0;
}

static int repeat_char(struct terminal* term, int args[], size_t num_args) {
	int arg = num_args ? args[0] : 1;
	if(term->last_char) {
		for(int i = 0; i < arg; i++) {
			tty_write(term, &term->last_char, 1);
		}
	}
	return 0;
}

static int reset(struct terminal* term, int args[], size_t num_args) {
	term->fg_color = FG_COLOR_DEFAULT;
	term->bg_color = BG_COLOR_DEFAULT;
	return 0;
}

static int set_bdc(struct terminal* term, char arg) {
	switch(arg) {
		case '0': term->write_bdc = true; break;
		case 'B': term->write_bdc = false; break;
		default: return -1;
	}
	return 0;
}

#define ESC_ADVANCE() {		\
	cur_str++;				\
	if(++spos >= str_len) {	\
		return 0;			\
	}						\
}

/* This function gets called by the tty output handlers whenever they encounter
 * Esc (\033). It returns the number of handled characters. If an escape code
 * is not recognized, this returns 0 so the code is printed to the screen.
 */
size_t tty_handle_escape_seq(struct terminal* term, char* str, size_t str_len) {
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
			term->cur_row--;
			result = 0;
			break;
		case 'B': // CUD
			term->cur_row++;
			result = 0;
			break;
		case 'C': // CUF
		// FIXME
		case '@': // ICH
			term->cur_col++;
			result = 0;
			break;
		case 'D': // CUB
			term->cur_col--;
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
