/* panic.c: Handle kernel panics
 * Copyright © 2011-2018 Lukas Martini
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include "panic.h"

#include "generic.h"
#include "print.h"
#include <tty/tty.h>
#include <hw/interrupts.h>
#include <hw/pit.h>
#include <hw/serial.h>
#include <string.h>
#include <mem/vmem.h>
#include <mem/kmalloc.h>
#include <spinlock.h>
#include <stdarg.h>

// lib/walk_stack.asm
extern int walk_stack(intptr_t* addresses, int naddr);
static spinlock_t lock;

static void panic_printf(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	serial_vprintf(fmt, va);
	va_end(va);
}

static void stacktrace() {
	intptr_t addresses[10];
	int read = walk_stack(addresses, 10);

	for(int i = 0; i < read; i++) {
		panic_printf("#%-16d0x%x\n", i, addresses[i]);
	}
}

void __attribute__((optimize("O0"))) panic(char* error, ...) {
	if(unlikely(!spinlock_get(&lock, 30))) {
		freeze();
	}

	va_list va;
	va_start(va, error);
	interrupts_disable();

	panic_printf("\nKernel Panic: ");
	vprintf(error, va);
	serial_vprintf(error, va);
	panic_printf("\n");
	va_end(va);

	panic_printf("Last PIT tick:   %d (rate %d, uptime: %d seconds)\n",
		(uint32_t)pit_tick, PIT_RATE, uptime());

	task_t* task = scheduler_get_current();
	if(task) {
		panic_printf("Running task:    %d <%s>", task->pid, task->name);

	/*	uint32_t task_offset = task->state->eip - task->entry;
		if(task_offset >= 0) {
			panic_printf("+%x at 0x%x", task_offset, task->state->eip);
		}
*/
		panic_printf("\n");
	} else {
		panic_printf("Running task:    [No task running]\n");
	}

	panic_printf("Paging context:  %s\n\n", vmem_get_name(vmem_currentContext));

	panic_printf("Call trace:\n");
	stacktrace();
	freeze();
}

void __stack_chk_fail(void) {
	panic("Stack protector failure\n");
}
void __stack_chk_guard(void) {
	panic("Stack protector guard failure\n");
}
