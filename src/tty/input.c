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

/* Convert tty_input_state/keycodes to ASCII character. Also converts a number of
 * single-byte escape sequences that are used in both canonical and non-canonical
 * mode.
 */
static inline char convert_to_char(struct tty_input_state* input) {
	if(input->code >= 512) {
		return 0;
	}

	char* chr = &tty_keymap_en[input->code];
	switch(*chr) {
		case 0x8:
		case 0x7f:
			return term->termios.c_cc[VERASE];
		case '\n':
			return term->termios.c_cc[VEOL];
	}

	if((input->control_left || input->control_right) && *chr >= 'a' && *chr <= 'z') {
		switch(*chr) {
			case 'd': return term->termios.c_cc[VEOF];
			case 'c': return term->termios.c_cc[VINTR];
			default: return *chr - 'a' + 1;
		}
	}

	// Shift into uppercase range of keymap if necessary
	if((input->shift_left || input->shift_right) ^ input->capslock) {
		chr += 256;
	}
	return *chr;
}

/* Canonical read mode input handler. Perform internal line-editing and block
 * read syscall until we encounter VEOF, VEOL, or the buffer is full.
 */
static void handle_canon(struct tty_input_state* input) {
	char chr = convert_to_char(input);
	if(!chr) {
		return;
	}

	// EOF / ^D
	if(chr == term->termios.c_cc[VEOF]) {
		term->read_done = true;
		return;
	}

	// Signal task on ^C
	if(chr == term->termios.c_cc[VINTR] && term->fg_task && term->termios.c_lflag & ISIG) {
		task_signal(term->fg_task, NULL, SIGINT, NULL);
	}

	if(chr == term->termios.c_cc[VERASE]) {
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

	if(chr == term->termios.c_cc[VEOL] || term->read_len >= sizeof(term->read_buf)) {
		term->read_done = true;
	}
}

/* Non-canonical read mode: Return immediately upon input regardless of
 * buffer size, and also return ECMA-48 encoded versions of control characters.
 */
static void handle_noncanon(struct tty_input_state* input) {
	char* inputseq;
	char char_p[1];

	size_t inputlen = 3;
	switch(input->code) {
		case 0x48: inputseq = "\033[A"; break; // Up arrow
		case 0x50: inputseq = "\033[B"; break; // Down arrow
		case 0x4d: inputseq = "\033[C"; break; // Right arrow
		case 0x4b: inputseq = "\033[D"; break; // Left arrow
		case 0x4f: inputseq = "\033[F"; break; // End
		case 0x47: inputseq = "\033[H"; break; // Home
		case 0x53: inputseq = "\033[3~"; break; // Del
		case 0x49: inputseq = "\033[5"; break;
		case 0x51: inputseq = "\033[6"; break;
		case 0xbb: inputseq = "\217[P"; break; // F1
		case 0xbc: inputseq = "\217[Q"; break; // F2
		case 0xbd: inputseq = "\217[R"; break; // F3
		case 0xbe: inputseq = "\217[S"; break; // F4
		case 0xbf: inputseq = "\033[15~"; break; // F5
		case 0xc0: inputseq = "\033[17~"; break; // F6
		case 0xc1: inputseq = "\033[18~"; break; // F7
		case 0xc2: inputseq = "\033[19~"; break; // F8
		case 0xc3: inputseq = "\033[20~"; break; // F9
		case 0xc4: inputseq = "\033[21~"; break; // F10
		case 0xd7: inputseq = "\033[23~"; break; // F11
		case 0xd8: inputseq = "\033[24~"; break; // F12*/
		default:
			*char_p = convert_to_char(input);
			if(!*char_p) {
				return;
			}

			inputseq = char_p;
			inputlen = 1;
	}

	memcpy(term->read_buf, inputseq, inputlen);
	term->read_len += inputlen;
	if(term->termios.c_lflag & ECHO) {
		tty_write(inputseq, inputlen);
	}

	term->read_done = true;
}

void tty_input_cb(struct tty_input_state* input) {
	if(!term || !input || term->read_len >= sizeof(term->read_buf)) {
		return;
	}

	if(term->termios.c_lflag & ICANON) {
		handle_canon(input);
	} else {
		handle_noncanon(input);
	}
}

size_t tty_read(char* dest, size_t size) {
	while(!term->read_done) {
		halt();
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
