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

#define flush() { while(inb(0x64) & 1) { inb(0x60); }}
#define send(c) { while((inb(0x64) & 0x2)); outb(0x60, (c)); }

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

	if(unlikely(escape_wait)) {
		escape_wait = false;
		tty_input_cb(0xe0, code);
	} else {
		tty_input_cb(code, 0);
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
	interrupts_register(IRQ1, &intr_handler, true);
}
