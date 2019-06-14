/* input.c: Terminal input handling
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
#include <stdlib.h>
#include <errno.h>

/* Convert tty_input_state/keycodes to ASCII character. Also converts a number of
 * single-byte escape sequences that are used in both canonical and non-canonical
 * mode.
 */
static inline char convert_to_char(struct terminal* term, struct tty_input_state* input) {
	if(input->code >= ARRAY_SIZE(tty_keymap_en)) {
		return 0;
	}

	char* chr = &tty_keymap_en[input->code];
	switch(*chr) {
		case '\b':
			return term->termios.c_cc[VERASE];
		case '\n':
			return term->termios.c_cc[VEOL];
	}

	// Map presses of Ctrl+[a-z] to ASCII chars 1-26
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
static void handle_canon(struct terminal* term, struct tty_input_state* input) {
	char chr = convert_to_char(term, input);
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
		tty_write(term, &chr, 1);
	}

	if(chr == term->termios.c_cc[VEOL] || term->read_len >= sizeof(term->read_buf)) {
		term->read_done = true;
	}
}

/* Non-canonical read mode: Return immediately upon input regardless of
 * buffer size, and also return ECMA-48 encoded versions of control characters.
 */
static void handle_noncanon(struct terminal* term, struct tty_input_state* input) {
	char* inputseq;
	char char_p[1];

	// Handle special keys that send escape sequences
	size_t inputlen;
	switch(input->code) {
		case 0x48: inputlen=3; inputseq = "\e[A"; break;   // Up arrow
		case 0x50: inputlen=3; inputseq = "\e[B"; break;   // Down arrow
		case 0x4d: inputlen=3; inputseq = "\e[C"; break;   // Right arrow
		case 0x4b: inputlen=3; inputseq = "\e[D"; break;   // Left arrow
		case 0x4f: inputlen=3; inputseq = "\e[F"; break;   // End
		case 0x47: inputlen=3; inputseq = "\e[H"; break;   // Home
		case 0x49: inputlen=3; inputseq = "\e[5"; break;   // Page up
		case 0x51: inputlen=3; inputseq = "\e[6"; break;   // Page down
		case 0x52: inputlen=4; inputseq = "\e[2~"; break;  // Insert
		case 0x53: inputlen=4; inputseq = "\e[3~"; break;  // Del
		case 0xbb: inputlen=3; inputseq = "\217[P"; break; // F1
		case 0xbc: inputlen=3; inputseq = "\217[Q"; break; // F2
		case 0xbd: inputlen=3; inputseq = "\217[R"; break; // F3
		case 0xbe: inputlen=3; inputseq = "\217[S"; break; // F4
		case 0xbf: inputlen=5; inputseq = "\e[15~"; break; // F5
		case 0xc0: inputlen=5; inputseq = "\e[17~"; break; // F6
		case 0xc1: inputlen=5; inputseq = "\e[18~"; break; // F7
		case 0xc2: inputlen=5; inputseq = "\e[19~"; break; // F8
		case 0xc3: inputlen=5; inputseq = "\e[20~"; break; // F9
		case 0xc4: inputlen=5; inputseq = "\e[21~"; break; // F10
		case 0xd7: inputlen=5; inputseq = "\e[23~"; break; // F11
		case 0xd8: inputlen=5; inputseq = "\e[24~"; break; // F12
		default:
			*char_p = convert_to_char(term, input);
			if(!*char_p) {
				return;
			}

			inputseq = char_p;
			inputlen = 1;
	}

	memcpy(term->read_buf, inputseq, inputlen);
	term->read_len += inputlen;
	if(term->termios.c_lflag & ECHO) {
		tty_write(term, inputseq, inputlen);
	}

	term->read_done = true;
}

// Called by keyboard interrupt handler.
void tty_input_cb(struct tty_input_state* input) {
	if(!active_tty || !input || active_tty->read_len >= sizeof(active_tty->read_buf)) {
		return;
	}

	// Check for ctrl-alt-f1 etc
	if((input->control_left || input->control_right) &&
		(input->alt_left || input->alt_right) &&
		input->code >= 0xbb && input->code <= 0xc4) {

		tty_switch(input->code - 0xbb);
		return;
	}

	if(active_tty->termios.c_lflag & ICANON) {
		handle_canon(active_tty, input);
	} else {
		handle_noncanon(active_tty, input);
	}
}

size_t tty_read(struct terminal* term, char* dest, size_t size) {
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

int tty_poll(vfs_file_t* fp, int events) {
	struct terminal* term = tty_from_path(fp->mount_path, fp->task, NULL);
	if(!term) {
		sc_errno = EINVAL;
		return -1;
	}

	if(events & POLLIN && term->read_len) {
		return POLLIN;
	}
	return 0;
}
