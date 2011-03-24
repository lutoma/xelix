/* interrupts.c: Initialization of and interface to interrupts.
 * Copyright © 2010 Christoph Sünderhauf
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
#include <lib/log.h>
#include <lib/generic.h>
#include <interrupts/idt.h>

#define EOI_MASTER 0x20
#define EOI_SLAVE  0xA0

interruptHandler_t interruptHandlers[256];

// Send EOI (end of interrupt) signals to the PICs.
static void sendEOI(uint8 which)
{
	outb(which, 0x20);
}

void __cdecl interrupts_callback(cpu_state_t regs)
{
	// That might look useless, but trust me, it isn't.
	static bool inInterrupt = false;
	
	if(inInterrupt)
		return; // Drop interrupt
	inInterrupt = true;

	// Is this an IRQ?
	if(regs.interrupt > 31)
	{
		// If this IRQ involved the slave, send a EOI to the slave.
		if (regs.interrupt >= 40)
			sendEOI(EOI_SLAVE);

		sendEOI(EOI_MASTER); // Master
	}

	interruptHandler_t handler = interruptHandlers[regs.interrupt];

	if(handler != NULL)
		handler(regs);
	
	inInterrupt = false;
}

void interrupts_registerHandler(uint8 n, interruptHandler_t handler)
{
	interruptHandlers[n] = handler;
	
	if(n > 31)
		log("interrupts: Registered interrupt handler for IRQ %d.\n", n - 32);
	else
		log("interrupts: Registered interrupt handler for %d.\n", n);
}

void interrupts_bulkRegisterHandler(uint8 start, uint8 end, interruptHandler_t handler)
{
		if(start >= end)
		{
			log("interrupts: Warning: Attempt to bulk register interrupt handlers with start >= end.\n");
			return;
		}
		
		sint32 i;
		for(i = start; i <= end; i++)
			interruptHandlers[i] = handler;

		if(start > 31)
			log("interrupts: Registered interrupt handler for IRQs %d -  %d.\n", start - 32, end - 32);
		else
			log("interrupts: Registered interrupt handlers for %d - %d.\n", start, end);
}

void interrupts_init()
{
	idt_init();

	// set all interruptHandlers to NULL.
	memset(interruptHandlers, NULL, 256 * sizeof(interruptHandler_t));
	log("interrupts: Initialized\n");
}
