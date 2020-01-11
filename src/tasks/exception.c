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
#include <mem/vmem.h>

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
	snprintf(message, 300, "for %s to 0x%x at %#x%s%s%s",
		state->err_code & PFE_WRITE ? "write" : "read",
		state->cr2, eip,
		state->err_code & PFE_PRES ? " (page present)" : "",
		state->err_code & PFE_RES ? " (reserved write)" : "",
		state->err_code & PFE_INST ? " (instruction fetch)" : "");

	if(state->err_code & PFE_USER) {
		// Some task page faults can be gracefully handled (stack allocations)
		if(task_page_fault_cb(task, state->cr2) == 0) {
			return;
		}

		log(LOG_WARN, "Page fault in task %d <%s> %s\n", task->pid,
			task->name, message);

		struct vmem_range* range = vmem_get_range(task->vmem_ctx, state->cr2, false);
		if(range) {
			log(LOG_WARN, "  phys: %#x, flags: ro %d, user %d\n",
				vmem_translate_ptr(range, state->cr2, false),
				range->readonly, range->user);

			log(LOG_WARN, "  vmem range: virt %#x -> %#x  phys %#x -> %#x\n",
				range->virt_start, range->virt_start + range->length,
				range->phys_start, range->phys_start + range->length);
		} else {
			log(LOG_WARN, "  No matching vmem range found.\n");
		}

		task_signal(task, NULL, SIGSEGV, NULL);
	} else {
		panic("Page fault %s\n", message);
	}
}

static void int_handler(task_t* task, isf_t* state, int num) {
	if(num >= ARRAY_SIZE(exceptions)) {
		panic("Unknown CPU exception %d", num);
	}

	struct exception exc = exceptions[num];
	void* eip = ((iret_t*)state->esp)->eip;

	if(num == 14) {
		handle_page_fault(task, state, eip);
		return;
	}

	if(!exc.signal || !task) {
		panic(exc.name);
	}

	log(LOG_WARN, "%s in task %d <%s> at %#x\n", exc.name, task->pid, task->name, eip);
	task_signal(task, NULL, exc.signal, NULL);
}

void task_exception_init() {
	int_register_bulk(0, 31, int_handler, true);
}
