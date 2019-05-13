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
#include <printf.h>
#include <tty/tty.h>
#include <int/int.h>
#include <bsp/timer.h>
#include <tty/serial.h>
#include <string.h>
#include <mem/vmem.h>
#include <mem/kmalloc.h>
#include <spinlock.h>
#include <stdarg.h>
#include <boot/multiboot.h>

static spinlock_t lock;
// Should seed this using RNG
uintptr_t __stack_chk_guard = 0xcafed00d;

#define panic_printf(fmt, args...) { \
	serial_printf(fmt, ## args);     \
	printf(fmt, ## args);            \
}

char* addr2name(intptr_t address) {
#ifdef __i386__
	size_t symtab_length;
	size_t strtab_length;
	size_t loff = -1;
	char* name = "?";

	struct elf_sym* symtab = multiboot_get_symtab(&symtab_length);
	char* strtab = multiboot_get_strtab(&strtab_length);
	if(!symtab_length || !strtab_length) {
		return "?";
	}

	for(struct elf_sym* sym = symtab; (intptr_t)sym < (intptr_t)symtab + symtab_length; sym++) {
		if(sym->name == 0 || sym->value == 0 || sym->value > address) {
			continue;
		}

		size_t offset = address - sym->value;
		if(offset < 0x3000 && offset < loff) {
			loff = offset;
			name = strtab + sym->name;
		}
	}

	return name;
#else
	return "?";
#endif
}

void __attribute__((optimize("O0"))) panic(char* fmt, ...) {
	if(unlikely(!spinlock_get(&lock, 30))) {
		freeze();
	}

	va_list va;
	va_start(va, fmt);
	interrupts_disable();

	char error[500];
	vsnprintf(error, 500, fmt, va);
	panic_printf("\nKernel Panic: %s\n", error);
	va_end(va);

	panic_printf("Last PIT tick:   %d (rate %d, uptime: %d seconds)\n",
		(uint32_t)timer_tick, timer_rate, uptime());

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

	#ifdef __i386__
	panic_printf("Call trace:\n");
	intptr_t addresses[10];
	int read = walk_stack(addresses, 10);

	for(int i = 0; i < read; i++) {
		panic_printf("#%-6d %s <%#x>\n", i, addr2name(addresses[i]), addresses[i]);
	}
	#endif

	freeze();
}

void __stack_chk_fail(void) {
	panic("Stack protector failure\n");
}
