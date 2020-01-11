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

#include <int/int.h>
#include <tty/tty.h>
#include <tty/input.h>
#include <portio.h>
#include <random.h>

#define flush() { while(inb(0x64) & 1) { inb(0x60); }}
#define send(c) { while((inb(0x64) & 0x2)); outb(0x60, (c)); }

static void intr_handler(task_t* task, isf_t* isf_state, int num) {
	static struct tty_input_state state;
	state.code = (uint16_t)inb(0x60);
	random_seed(state.code + timer_tick);

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

	tty_input_cb(&state);
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
	int_register(IRQ(1), &intr_handler, false);
}
