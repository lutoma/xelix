/* exception.c: Handle CPU exceptions
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
#include <int/int.h>
#include <tasks/task.h>
#include <mem/mem.h>

// Page fault error code flags
#define PFE_PRES  1
#define PFE_WRITE 2
#define PFE_USER  4
#define PFE_RES   8
#define PFE_INST  16

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

static inline void handle_page_fault(task_t* task, isf_t* state, void* eip) {
	char message[300];
	snprintf(message, 300, "for %s to %p at %p%s%s%s",
		state->err_code & PFE_WRITE ? "write" : "read",
		state->cr2, eip,
		state->err_code & PFE_PRES ? " (page present)" : "",
		state->err_code & PFE_RES ? " (reserved write)" : "",
		state->err_code & PFE_INST ? " (instruction fetch)" : "");

	if(task && (state->err_code & PFE_USER)) {
		// Some task page faults can be handled gracefully
		// (Copy on write, stack allocations)
		if(task_page_fault_cb(task, state->cr2) == 0) {
			return;
		}

		log(LOG_WARN, "Page fault in task %d <%s> %s\n", task->pid,
			task->name, message);

		vm_alloc_t* range = vm_get(&task->vmem, state->cr2, false);
		if(range) {
			log(LOG_WARN, "  phys: %p, flags: rw %d, user %d\n",
				valloc_translate_ptr(range, state->cr2, false),
				range->flags & VM_RW, range->flags & VM_USER);

			log(LOG_WARN, "  vmem range: virt %p -> %p  phys %p -> %p\n",
				range->addr, range->addr + range->size,
				range->phys, range->phys + range->size);
		} else {
			log(LOG_WARN, "  No matching vmem range found.\n");
		}

		task_signal(task, NULL, SIGSEGV, NULL);
	} else {
		panic("Page fault %s", message);
	}
}

static void int_handler(task_t* task, isf_t* state, int num) {
	if(num >= ARRAY_SIZE(exceptions)) {
		panic("Unknown CPU exception %d", num);
	}

	struct exception exc = exceptions[num];
	void* eip = NULL;
	if(state && state->esp) {
		eip = ((iret_t*)state->esp)->eip;
	}

	if(num == 14) {
		handle_page_fault(task, state, eip);
		return;
	}

	if(!exc.signal || !task || task->task_state == TASK_STATE_SYSCALL) {
		panic("%s at %p\n", exc.name, eip);
	}

	log(LOG_WARN, "%s in task %d <%s> at %p\n", exc.name, task->pid, task->name, eip);
	task_signal(task, NULL, exc.signal, NULL);
}

void task_exception_init() {
	int_register_bulk(0, 31, int_handler, true);
}
