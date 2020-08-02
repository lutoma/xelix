/* keyboard.c: A generic PS2 keyboard driver.
 *
 * Copyright © 2019-2020 Lukas Martini
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

#include <int/int.h>
#include <tty/tty.h>
#include <tty/input.h>
#include <block/random.h>
#include <fs/sysfs.h>
#include <portio.h>
#include <buffer.h>
#include <errno.h>
#include <tty/keymap.h>

#define flush() { while(inb(0x64) & 1) { inb(0x60); }}
#define send(c) { while((inb(0x64) & 0x2)); outb(0x60, (c)); }

static struct buffer* buf = NULL;

/* Convert tty_input_state/keycodes to ASCII character. Also converts a number of
 * single-byte escape sequences that are used in both canonical and non-canonical
 * mode.
 */
static inline char convert_to_char(struct tty_input_state* input) {
	if(input->code >= ARRAY_SIZE(tty_keymap_en)) {
		return 0;
	}

	char* chr = &tty_keymap_en[input->code];

	// Map presses of Ctrl+[a-z] to ASCII chars 1-26
	if((input->control_left || input->control_right) && *chr >= 'a' && *chr <= 'z') {
		switch(*chr) {
			case 'd': return 4;
			case 'c': return 3;
			default: return *chr - 'a' + 1;
		}
	}

	// Shift into uppercase range of keymap if necessary
	if((input->shift_left || input->shift_right) ^ input->capslock) {
		chr += 256;
	}
	return *chr;
}


/* Non-canonical read mode: Return immediately upon input regardless of
 * buffer size, and also return ECMA-48 encoded versions of control characters.
 */
static void handle_noncanon(struct tty_input_state* input) {
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
		case 0xd2: inputlen=3; inputseq = "\e[L"; break; // Insert
		case 0x49: inputlen=4; inputseq = "\e[5~"; break;   // Page up
		case 0x51: inputlen=4; inputseq = "\e[6~"; break;   // Page down
		case 0x52: inputlen=4; inputseq = "\e[2~"; break;  // Insert
		case 0x53: inputlen=4; inputseq = "\e[3~"; break;  // Del
		case 0xbb: inputlen=5; inputseq = "\e[11~"; break; // F1
		case 0xbc: inputlen=5; inputseq = "\e[12~"; break; // F2
		case 0xbd: inputlen=5; inputseq = "\e[13~"; break; // F3
		case 0xbe: inputlen=5; inputseq = "\e[14~"; break; // F4
		case 0xbf: inputlen=5; inputseq = "\e[15~"; break; // F5
		case 0xc0: inputlen=5; inputseq = "\e[17~"; break; // F6
		case 0xc1: inputlen=5; inputseq = "\e[18~"; break; // F7
		case 0xc2: inputlen=5; inputseq = "\e[19~"; break; // F8
		case 0xc3: inputlen=5; inputseq = "\e[20~"; break; // F9
		case 0xc4: inputlen=5; inputseq = "\e[21~"; break; // F10
		case 0xd7: inputlen=5; inputseq = "\e[23~"; break; // F11
		case 0xd8: inputlen=5; inputseq = "\e[24~"; break; // F12
		default:
			*char_p = convert_to_char(input);
			if(!*char_p) {
				return;
			}

			inputseq = char_p;
			inputlen = 1;
	}

	buffer_write(buf, inputseq, inputlen);
}

static void intr_handler(task_t* task, isf_t* isf_state, int num) {
	static struct tty_input_state state;
	state.code = (uint16_t)inb(0x60);
	block_random_seed(state.code + timer_tick);

	// Escape sequences consist of two scancodes: One first that tells us
	// we're now in an escape sequence, and the second one with the actual
	// sequence.
	static bool escape_wait;
	if(unlikely(state.code == 0xe0)) {
		if(escape_wait) {
			state.code += 0x1000;
		}
		escape_wait = !escape_wait;
		return;
	}

	switch(state.code) {
		case 0x2a:
			state.shift_left = true; return;
		case 0x36:
			state.shift_right = true; return;
		case 0xaa:
			state.shift_left = false; return;
		case 0xb6:
			state.shift_right = false; return;
		case 0x1d:
			state.control_left = true; return;
		case 0x3a:
			state.capslock = !state.capslock; return;
		case 0x101d:
			state.control_right = true; return;
		case 0x9d:
			state.control_left = false; return;
		case 0x109d:
			state.control_right = false; return;
		case 0x38:
			state.alt_left = true; return;
		case 0x1038:
			state.alt_right = true; return;
		case 0xb8:
			state.alt_left = false; return;
		case 0x10b8:
			state.alt_right = false; return;
	}

	handle_noncanon(&state);
	tty_input_cb(&state);
}


static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(!buffer_size(buf) && ctx->fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

	while(!buffer_size(buf)) {
		halt();
	}

	return buffer_pop(buf, dest, size);
}

static int sfs_poll(struct vfs_callback_ctx* ctx, int events) {
	if(events & POLLIN && buffer_size(buf)) {
		return POLLIN;
	}
	return 0;
}

void tty_keyboard_init() {
	buf = buffer_new(10);
	flush();

	// Reset
	send(0xF6);

	// Repeat rate
	send(0xF3);
	send(0);

	send(0xF4);
	flush();
	int_register(IRQ(1), &intr_handler, false);

	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
		.poll = sfs_poll,
	};
	sysfs_add_dev("keyboard1", &sfs_cb);
}
