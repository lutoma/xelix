/* fault.c: Catch and process CPU fault interrupts
 * Copyright © 2010-2016 Lukas Martini
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
#include <tasks/scheduler.h>

static char* exception_names[] = {
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

static void handler(cpu_state_t* regs) {
	task_t* proc = scheduler_get_current();
	char* error_name = "Unknown CPU error";

	if(regs->interrupt < 19) {
		error_name = exception_names[regs->interrupt];
	}

	// Always do a full panic on double faults
	if(proc && regs->interrupt != 8) {

		log(LOG_WARN, "%s in process <%s>+%x at EIP 0x%x of context %s. Terminating the task.\n",
			error_name, proc->name, (regs->eip - (uint32_t)proc->entry),
			regs->eip, vmem_get_name(proc->memory_context));

		scheduler_terminate_current();
		return;
	}

	panic(error_name);
}

void cpu_init_fault_handlers() {
	// Interrupt 14 (page fault) is handled by memory/vmem.c
	interrupts_bulk_register(0, 13, &handler);
	interrupts_bulk_register(15, 0x1F, &handler);
}
