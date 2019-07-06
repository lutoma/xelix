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
#include <int/int.h>
#include <tasks/task.h>

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

static void int_handler(task_t* task, isf_t* state, int num) {
	if(num >= ARRAY_SIZE(exceptions)) {
		panic("Unknown CPU exception %d", num);
	}

	struct exception exc = exceptions[num];

	// For page faults, bit 2 of the error code is unset if kernel ctx
	if(num == 14 && !(state->err_code & 0b100)) {
		panic("Page fault for %s to 0x%x%s%s%s\n",
			state->err_code & 2 ? "write" : "read", state->cr2,
			state->err_code & 1 ? " (page present)" : "",
			state->err_code & 4 ? " (reserved write)" : "",
			state->err_code & 16 ? " (instruction fetch)" : "");
	}

	if(!exc.signal || !task) {
		panic(exc.name);
	}

	if(num == 14) {
		log(LOG_WARN, "Page fault in task %d %s for %s to 0x%x%s\n",
			task->pid, task->name, state->err_code & 2 ? "write" : "read",
			state->cr2, state->err_code & 1 ? " (page present)" : "");
	} else {
		log(LOG_WARN, "%s in task %d %s\n", exc.name, task->pid, task->name);
	}

	task_signal(task, NULL, exc.signal, NULL);
}

void task_exception_init() {
	interrupts_bulk_register(0, 31, int_handler, true);
}
