/* interrupts.c: Initialization of and interface to interrupts.
 * Copyright © 2010 Christoph Sünderhauf
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
#include <common/log.h>
#include <common/generic.h>
#include <interrupts/idt.h>

interruptHandler_t interruptHandlers[256];

void interrupt_callback(registers_t regs)
{
	// That might look useless, but trust me, it isn't.
	static bool inInterrupt = false;
	
	if(inInterrupt)
		return; // Drop interrupt
	inInterrupt = true;
	
	if (interruptHandlers[regs.int_no] != 0)
	{
		interruptHandler_t handler = interruptHandlers[regs.int_no];
		handler(regs);
	}
	
	inInterrupt = false;
}

void interrupt_registerHandler(uint8 n, interruptHandler_t handler)
{
	interruptHandlers[n] = handler;
	log("interrupts: Registered IRQ handler for %d.\n", n);
}

void interrupts_init()
{
	idt_init();

	// set all interruptHandlers to zero
	memset(interruptHandlers, 0, 256*sizeof(interruptHandler_t));
	log("interrupts: Initialized\n");
}
