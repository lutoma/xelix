/* tty.c: Terminal input handling
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

#include <tty/input.h>
#include <tty/tty.h>

void tty_input_cb(struct tty_input_state* input) {
	if(!term || !input || !term->read_buf || term->read_len >= term->read_buf_size) {
		return;
	}

	if(input->control_left || input->control_right) {
		switch(input->chr) {
			case 'd':
			case 'D':
				if(term->termios.c_lflag & ICANON) {
					term->read_done = true;
					return;
				}

				input->chr = term->termios.c_cc[VEOF]; break;
			case 'c':
			case 'C':
				input->chr = term->termios.c_cc[VINTR]; break;
		}
	} else {
		switch(input->chr) {
			case 0x8:
			case 0x7f:
				input->chr = term->termios.c_cc[VERASE]; break;
			case '\n':
				input->chr = term->termios.c_cc[VEOL]; break;
		}
	}

	if(input->chr == term->termios.c_cc[VERASE] && term->termios.c_lflag & ICANON) {
		if(!term->read_len) {
			return;
		}

		term->read_len--;
	} else {
		term->read_buf[term->read_len++] = input->chr;
	}

	if(term->termios.c_lflag & ECHO) {
		tty_put_char(input->chr);
	}

	if(input->chr == term->termios.c_cc[VEOL] || term->read_len >= term->read_buf_size) {
		term->read_done = true;
	}
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
