/* generic.c: Interface to the programmable interrupt timer
 * Copyright © 2010-2018 Lukas Martini
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

#include "pit.h"
#include <hw/interrupts.h>
#include <fs/sysfs.h>

static uint32_t tick = 0;

// The timer callback. Gets called every time the PIT fires.
static void timer_callback(cpu_state_t* regs) {
	tick++;
}

uint32_t pit_get_tick(void) {
	return tick;
}

static size_t sfs_read(void* dest, size_t size) {
	size_t rsize = 0;
	sysfs_printf("%d %d %d", uptime(), tick, (PIT_RATE));
	return rsize;
}

// Initialize the PIT
void pit_init(uint16_t frequency)
{
	interrupts_register(IRQ0, &timer_callback);

	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency. Important to note is
	// that the divisor must be small enough to fit into 16-bits.
	uint32_t divisor = 1193180 / frequency;
	// Send the command byte.
	outb(0x43, 0x36);

	// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
	uint8_t l = (uint8_t)(divisor & 0xFF);
	uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

	// Send the frequency divisor.
	outb(0x40, l);
	outb(0x40, h);

	sysfs_add_file("tick", sfs_read, NULL);
}
