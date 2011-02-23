/* irq.c: Handling of IRQs
 * Copyright © 2011 Lukas Martini
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

#include "interface.h"

// Send EOI (end of interrupt) signals to the PICs.
static void sendEOI(bool slave)
{
	if (slave)
		outb(0xA0, 0x20);
	else
		outb(0x20, 0x20);
}

// This gets called from our ASM irq handler stub.
void irq_handler(registers_t regs)
{
	// If this interrupt involved the slave, send a EOI to the slave.
	if (regs.int_no >= 40)
		sendEOI(true);

	sendEOI(false); // Master
	interrupt_callback(regs);
}
