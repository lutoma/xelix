/* fault.c: Catch and process CPU fault interrupts
 * Copyright © 2010-2015 Lukas Martini
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

#include "cpu.h"

#include <lib/log.h>
#include <lib/panic.h>
#include <hw/interrupts.h>
#include <memory/vmem.h>
#include <memory/paging.h>

static char* exception_names[] =
{
	"Division by zero",
	"Debug exception",
	"Non maskable interrupt",
	"Breakpoint",
	"Into detected",
	"Out of bounds",
	"Invalid opcode",
	"No coprocessor",
	"Double fault",
	"Bad TSS",
	"Segment not present",
	"Stack fault",
	"General protection fault",
	"Unknown interrupt exception",
	"Page fault",
	"Coprocessor fault",
	"Alignment check exception",
	"Machine check exception"
};

static void handle_page_fault(cpu_state_t *regs)
{
	int cr2;
	asm volatile("mov %0, cr2":"=r"(cr2));
	vmem_handle_fault(regs->errCode, regs->eip);
}

static void handler(cpu_state_t* regs)
{
	if(regs->interrupt > 18) {
		panic("Unkown CPU error\n");
	} else if(regs->interrupt == 14) {
		handle_page_fault(regs);
	} else {
		panic(exception_names[regs->interrupt]);
	}
}

void cpu_init_fault_handlers()
{
	interrupts_bulkRegisterHandler(0, 0x1F, &handler);
}
