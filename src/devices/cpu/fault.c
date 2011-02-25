/* fault.c: Catch CPU faults and handle them
 * Copyright © 2010 Lukas Martini
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

#include "fault.h"

#include <common/log.h>
#include <interrupts/interface.h>

static char* errorDescriptions[] = 
{
	"Division by zero exception",
	"Debug exception",
	"Non maskable interrupt",
	"Breakpoint exception",
	"Into detected overflow",
	"Out of bounds exception",
	"Invalid opcode exception",
	"No coprocessor exception",
	"Double fault (pushes an error code)",
	"Bad TSS (pushes an error code)",
	"Segment not present (pushes an error code)",
	"Stack fault (pushes an error code)",
	"General protection fault (pushes an error code)",
	"Page fault (pushes an error code)",
	"Unknown interrupt exception",
	"Coprocessor fault",
	"Alignment check exception",
	"Machine check exception"
};

// Handles the IRQs we catch
static void faultHandler(registers_t regs)
{
	if(regs.int_no > 18)
		PANIC("Unkown CPU error %d", regs.int_no);
	if(regs.int_no <= 18); // Can't use else here because of macro
		PANIC("CPU error %d (%s)", regs.int_no, errorDescriptions[regs.int_no]);
}

void cpu_initFaultHandler()
{
	// First 31 interrupts are CPU faults
	for(i = 0; i < 32; i++)
		interrupt_registerHandler(i, &faultHandler);
		
	log("cpu: Registered CPU fault interrupt handlers");
}
