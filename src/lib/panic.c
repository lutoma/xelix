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
#include <int/int.h>
#include <bsp/timer.h>
#include <tty/serial.h>
#include <string.h>
#include <mem/kmalloc.h>
#include <spinlock.h>
#include <stdarg.h>
#include <boot/multiboot.h>
#include <gfx/fbtext.h>
#include <tasks/task.h>

void __stack_chk_fail(void);

static spinlock_t lock;
// Should seed this using RNG
uintptr_t __stack_chk_guard = 0xcafed00d;

#define panic_printf(fmt, args...) { \
	serial_printf(fmt, ## args);     \
	printf(fmt, ## args);            \
}

char* addr2name(intptr_t address) {
	size_t symtab_length;
	size_t strtab_length;

	struct elf_sym* symtab = multiboot_get_symtab(&symtab_length);
	char* strtab = multiboot_get_strtab(&strtab_length);
	if(!symtab_length || !strtab_length) {
		return "?";
	}

	struct elf_sym* match = NULL;
	for(struct elf_sym* sym = symtab; (uintptr_t)sym < (uintptr_t)symtab + symtab_length; sym++) {
		if(!sym->name || !sym->value || (match && sym->value < match->value)) {
			continue;
		}

		if(address >= sym->value) {
			match = sym;
		}
	}

	if(match) {
		return strtab + match->name;
	} else {
		return "?";
	}
}

void __attribute__((optimize("O0"))) _panic(char* fmt, ...) {
	if(unlikely(!spinlock_get(&lock, 30))) {
		freeze();
	}

	va_list va;
	va_start(va, fmt);
	int_disable();
	gfx_fbtext_show();


	char error[500];
	vsnprintf(error, 500, fmt, va);
	panic_printf("   \n--------------------\n");
	panic_printf("Kernel Panic: %s\n", error);
	va_end(va);
	panic_printf("   \n");
	panic_printf("Last tick:   %d (@%d/s)\n", timer_tick, timer_rate);

	task_t* task = scheduler_get_current();
	if(task) {
		panic_printf("Task:        %-7d (%s)\n", task->pid, task->name);
	} else {
		panic_printf("Task:        (None)\n");
	}

	panic_printf("   \n");
	panic_printf("Call trace:\n");
	intptr_t addresses[10];
	int read = walk_stack(addresses, 10);

	for(int i = 0; i < read; i++) {
		panic_printf("#%-6d %s <%x>\n", i, addr2name(addresses[i]), addresses[i]);
	}

	freeze();
}

void __stack_chk_fail(void) {
	panic("Stack protector failure\n");
}
