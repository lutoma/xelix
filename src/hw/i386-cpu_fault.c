/* fault.c: Catch and process CPU fault interrupts
 * Copyright © 2010-2019 Lukas Martini
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
#include <bitmap.h>
#include <hw/interrupts.h>
#include <mem/vmem.h>
#include <mem/paging.h>
#include <mem/gdt.h>
#include <tasks/scheduler.h>
#include <tasks/task.h>

#define has_errcode(i) (i == 8 || i == 17 || i == 30 || (i >= 10 && i <= 14))

struct exception {
	int signal;
	char* name;
};

static const struct exception exceptions[] = {
	{SIGFPE, "Division by zero"},
	{SIGTRAP, "Debug exception"},
	{0, "Non maskable interrupt"},
	{SIGTRAP, "Breakpoint"},
	{0, "Into overflow"},
	{SIGILL, "Out of bounds"},
	{SIGILL, "Invalid opcode"},
	{SIGFPE, "No coprocessor"},
	{0, "Double fault"},
	{0, "Coprocessor segment overrun"},
	{0, "Invalid TSS"},
	{0, "Segment not present"},
	{0, "Stack segment fault"},
	{SIGSEGV, "General protection fault"},
	{SIGSEGV, "Page fault"},
	{0, "Reserved exception"},
	{SIGFPE, "Coprocessor fault"},
	{0, "Alignment check exception"},
	{0, "Machine check exception"},
	{SIGFPE, "SIMD floating-point exception"},
	{0, "Virtualization exception"},
};

isf_t* __fastcall cpu_fault_handler(uint32_t intr, intptr_t cr2, uint32_t d1, uint32_t d2) {
	if(intr >= sizeof(exceptions) / sizeof(struct exception)) {
		panic("Unknown CPU exception");
	}

	struct exception exc = exceptions[intr];
	task_t* task = scheduler_get_current();
	uint32_t err_code;

	//uint32_t eip;
	if(has_errcode(intr)) {
		err_code = d1;
		//eip = d2;
	} else {
		err_code = 0;
		//eip = d1;
	}

	// For page faults, bit 2 of the error code is unset if kernel ctx
	if(intr == 14 && !(err_code & 0b100)) {
		panic("Page fault for %s to 0x%x%s\n",
			err_code & 2 ? "write" : "read", cr2,
			err_code & 1 ? " (page present)" : "");
	}

	if(!exc.signal || !task) {
		panic(exc.name);
	}

	task_signal(task, NULL, exc.signal, NULL);
	if(intr == 14) {
		log(LOG_WARN, "Page fault in task %d %s for %s to 0x%x%s\n",
			task->pid, task->name, err_code & 2 ? "write" : "read",
			cr2, err_code & 1 ? " (page present)" : "");
	} else {
		log(LOG_WARN, "%s in task %d %s\n", exc.name, task->pid, task->name);
	}

	task_t* new_task = scheduler_select(NULL);
	if(!new_task || !new_task->state) {
		panic("cpu_fault: No new task.\n");
	}

	gdt_set_tss(new_task->kernel_stack + PAGE_SIZE);
	return new_task->state;
}
