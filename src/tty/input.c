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
#include <tty/keymap.h>

inline void write_escape(char e) {
	term->read_buf[term->read_len++] = '\033';
	term->read_buf[term->read_len++] = '[';
	term->read_buf[term->read_len++] = e;
	term->read_done = true;
}

void tty_input_cb(struct tty_input_state* input) {
	if(!term || !input || term->read_len >= sizeof(term->read_buf)) {
		return;
	}

	if(!(term->termios.c_lflag & ICANON)) {
		switch(input->code) {
			case 0x48: write_escape('A'); return;
			case 0x50: write_escape('B'); return;
			case 0x4d: write_escape('C'); return;
			case 0x4b: write_escape('D'); return;
			case 0x4f: write_escape('F'); return;
			case 0x47: write_escape('H'); return;
			case 0x53: write_escape('3'); return;
			case 0x49: write_escape('5'); return;
			case 0x51: write_escape('6'); return;
		}
	}

	char chr = 0;
	if(input->code < 512) {
		if((input->shift_left || input->shift_right) ^ input->capslock) {
			chr = tty_keymap_en[input->code + 256];
		} else {
			chr = tty_keymap_en[input->code];
		}
	}

	if(!chr) {
		return;
	}

	switch(chr) {
		case 0x8:
		case 0x7f:
			chr = term->termios.c_cc[VERASE]; break;
		case '\n':
			chr = term->termios.c_cc[VEOL]; break;
	}

	if(input->control_left || input->control_right) {
		switch(chr) {
			case 'd':
			case 'D':
				if(term->termios.c_lflag & ICANON) {
					term->read_done = true;
					return;
				}

				chr = term->termios.c_cc[VEOF]; break;
			case 'c':
			case 'C':
				if(term->fg_task && term->termios.c_lflag & ISIG) {
					task_signal(term->fg_task, NULL, SIGINT, NULL);
					term->fg_task->interrupt_yield = true;
				}
				chr = term->termios.c_cc[VINTR];
				break;
		}
	}

	if(chr == term->termios.c_cc[VERASE] && term->termios.c_lflag & ICANON) {
		if(!term->read_len) {
			return;
		}

		term->read_len--;
	} else {
		term->read_buf[term->read_len++] = chr;
	}

	if(term->termios.c_lflag & ECHO) {
		tty_write(&chr, 1);
	}

	if(chr == term->termios.c_cc[VEOL] || !(term->termios.c_lflag & ICANON) || term->read_len >= sizeof(term->read_buf)) {
		term->read_done = true;
	}
}

size_t tty_read(char* dest, size_t size) {
	while(!term->read_done) {
		asm("hlt");
	}

	size = MIN(size, term->read_len);
	term->read_len -= size;
	memcpy(dest, term->read_buf, size);

	if(term->read_len) {
		memmove(term->read_buf, term->read_buf + size, term->read_len);
	} else {
		term->read_done = false;
	}
	return size;
}
