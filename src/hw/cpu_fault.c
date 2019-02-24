/* fault.c: Catch and process CPU fault interrupts
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

#include <log.h>
#include <panic.h>
#include <hw/interrupts.h>
#include <mem/vmem.h>
#include <mem/paging.h>
#include <mem/gdt.h>
#include <tasks/scheduler.h>
#include <tasks/task.h>

#define has_errcode(i) (i == 8 || i == 17 || i == 30 || (i >= 10 && i <= 14))

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

isf_t* __attribute__((fastcall)) cpu_fault_handler(uint32_t intr, intptr_t cr2, uint32_t d1, uint32_t d2) {
	uint32_t err_code;
	//uint32_t eip;
	if(has_errcode(intr)) {
		err_code = d1;
		//eip = d2;
	} else {
		err_code = 0;
		//eip = d1;
	}

	char* error_name = (intr < 19) ? exception_names[intr] : "Unknown CPU error";

	// Always do a full panic on double faults
	if(bit_get(err_code, 2) && intr != 8) {
		task_t* task = scheduler_get_current();
		if(task) {
			// FIXME Should signal SIGSEGV etc.
			task_exit(task);
		}

		log(LOG_ERR, "%s in task %d %s\n", error_name, scheduler_get_current()->pid, scheduler_get_current()->name);

		task_t* new_task = scheduler_select(NULL);
		if(!new_task || !new_task->state) {
			panic("cpu_fault: No new task.\n");
		}

		gdt_set_tss(new_task->kernel_stack + PAGE_SIZE);
		return new_task->state;
	}

	if(intr == 14) {
		panic("Page fault for %s to 0x%x %s\n",
			bit_get(err_code, 1) ? "write" : "read", cr2,
			bit_get(err_code, 0) ? " (page present)" : "");
	} else {
		panic(error_name);
	}
	__builtin_unreachable();
}

void cpu_init_fault_handlers() {
}
