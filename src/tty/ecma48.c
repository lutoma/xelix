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

static int set_char_attrs(char* intermediate) {
	if(!intermediate) {
		term->fg_color = FG_COLOR_DEFAULT;
		term->bg_color = BG_COLOR_DEFAULT;
		return 0;
	}

	char* last = intermediate;
	char* pos = NULL;
	do {
		pos = strchr(last, ';');
		int attr = atoi(last);

		if(attr == 0) {
			term->fg_color = FG_COLOR_DEFAULT;
			term->bg_color = BG_COLOR_DEFAULT;
		} else if(attr >= 30 && attr < 40) {
			term->fg_color = attr - 30;
		} else if(attr >= 40 && attr < 50) {
			term->bg_color = attr - 40;
		}

		last = pos + 1;
	} while(pos);
	return 0;
}

static int clear(char* intermediate) {
	int arg = 0;
	if(intermediate) {
		arg = atoi(intermediate);
	}

	switch(arg) {
		case 0:
			term->drv->clear(term->cur_col, term->cur_row, term->drv->cols, term->drv->rows); break;
		case 1:
			term->drv->clear(0, 0, term->cur_col, term->cur_row); break;
		case 2:
			term->drv->clear(0, 0, term->drv->cols, term->drv->rows); break;
	}
	return 0;
}

static int erase_in_line(char* intermediate) {
	int arg = 0;
	if(intermediate) {
		arg = atoi(intermediate);
	}

	switch(arg) {
		case 0:
			term->drv->clear(term->cur_col, term->cur_row, term->drv->cols, term->cur_row); break;
		case 1:
			term->drv->clear(0, term->cur_row, term->cur_col, term->cur_row); break;
		case 2:
			term->drv->clear(0, term->cur_row, term->drv->cols, term->cur_row); break;
	}
	return 0;
}

static int set_pos(char* intermediate) {
	if(!intermediate) {
		term->cur_col = 0;
		term->cur_row = 0;
		return 0;
	}

	char* pos = strchr(intermediate, ';');
	if(!pos) {
		return -1;
	}

	*pos = 0;
	term->cur_col = atoi(pos + 1) - 1;
	term->cur_row = atoi(intermediate) - 1;
	return 0;
}

static int set_row_pos(char* intermediate) {
	int arg = 0;
	if(intermediate) {
		arg = atoi(intermediate) - 1;
	}

	term->cur_row = arg;
	return 0;
}

static int set_col_pos(char* intermediate) {
	int arg = 0;
	if(intermediate) {
		arg = atoi(intermediate) - 1;
	}

	term->cur_col = arg;
	return 0;
}

static int clear_forward(char* intermediate) {
	int arg = 1;
	if(intermediate) {
		arg = atoi(intermediate);
	}

	term->drv->clear(term->cur_col, term->cur_row, term->cur_col + arg, term->cur_row);
	return 0;
}

static int reset(char* intermediate) {
	term->fg_color = FG_COLOR_DEFAULT;
	term->bg_color = BG_COLOR_DEFAULT;
	return 0;
}

#define ESC_ADVANCE(x) { cur_str += x; spos += x; }
size_t tty_handle_escape_seq(char* str, size_t str_len) {
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
	while(spos <= str_len && (*cur_str == ';' || *cur_str == '?' || ('0' <= *cur_str && '9' >= *cur_str))) {
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

	static int cnt = 0;
	serial_printf("escape code #%d, %c\n", cnt++, *cur_str);
	int result;
	switch(*cur_str) {
		case 'm': result = set_char_attrs(intermediate); break;
		case 'J': result = clear(intermediate); break;
		case 'K': result = erase_in_line(intermediate); break;
		case 'H': result = set_pos(intermediate); break;
		case 'd': result = set_row_pos(intermediate); break;
		case 'G': result = set_col_pos(intermediate); break;
		case 'X': result = clear_forward(intermediate); break;
		case 'l': result = reset(intermediate); break;
		default:
			serial_printf("Unknown escape code %c\n", *cur_str);
			result = -1;
			break;
	}

	if(intermediate) {
		kfree(intermediate);
	}
	return result == -1 ? 0 :spos;
}
