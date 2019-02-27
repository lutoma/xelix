/* keyboard.c: A generic PS2 keyboard driver.
 *
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

#include <hw/interrupts.h>
#include <tty/tty.h>
#include <tty/input.h>
#include <tty/keymap.h>
#include <portio.h>

#define flush() { while(inb(0x64) & 1) { inb(0x60); }}
#define send(c) { while((inb(0x64) & 0x2)); outb(0x60, (c)); }

static struct tty_input_state state;

static struct tty_input_state* keycode_to_input(uint8_t code, bool esc) {
	switch(code) {
		case 0x2a:
			state.shift_left = true; return NULL;
		case 0x36:
			state.shift_right = true; return NULL;
		case 0xaa:
			state.shift_left = false; return NULL;
		case 0xb6:
			state.shift_right = false; return NULL;
		case 0x1d:
			if(esc) {
				state.control_right = true;
			} else {
				state.control_left = true;
			}
			return NULL;
		case 0x9d:
			if(esc) {
				state.control_right = false;
			} else {
				state.control_left = false;
			}
			return NULL;
		case 0x38:
			if(esc) {
				state.alt_right = true;
			} else {
				state.alt_left = true;
			}
			return NULL;
		case 0xb8:
			if(esc) {
				state.alt_right = false;
			} else {
				state.alt_left = false;
			}
			return NULL;
	}

	if(esc) {
		serial_printf("unknown escaped code 0x%x\n", code);
		return NULL;
	}

	uint32_t rcode = code;
	if(state.shift_left || state.shift_right) {
		rcode += 256;
	}
	if(rcode > 512) {
		return NULL;
	}

	state.chr = tty_keymap_en[rcode];
	return &state;
}


static void intr_handler(isf_t* regs) {
	// Escape sequences consist of two scancodes: One first that tells us
	// we're now in an escape sequence, and the second one with the actual
	// sequence.
	static bool escape_wait;

	uint8_t code = inb(0x60);
	if(unlikely(code == 0xe0)) {
		escape_wait = true;
		return;
	}

	struct tty_input_state* input;
	if(unlikely(escape_wait)) {
		escape_wait = false;
		input = keycode_to_input(code, true);
	} else {
		input = keycode_to_input(code, false);
	}
	if(input && input->chr) {
		tty_input_cb(input);
	}
}

void tty_keyboard_init() {
	flush();

	// Reset
	send(0xF6);

	// Repeat rate
	send(0xF3);
	send(0);

	send(0xF4);
	flush();
	interrupts_register(IRQ1, &intr_handler, false);
}
