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
#include <console/console.h>
#include <hw/interrupts.h>
#include <hw/cpu.h>
#include <hw/pit.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <lib/string.h>
#include <memory/vmem.h>
#include <memory/kmalloc.h>

// lib/walk_stack.asm
extern int walk_stack(intptr_t* addresses, int naddr);

static void panic_printf(const char *fmt, ...) {
	vprintf(fmt, (void**)(&fmt) + 1);
	serial_vprintf(fmt, (void**)(&fmt) + 1);
}

/* Write to display/serial, completely circumventing the console framework and
 * device drivers. Writes directly to video memory / serial ioports.
 *
 * Ideally, the output of this will later then be overwritten by the full
 * output routed via the console framework.
 */
static void bruteforce_print(char* chars) {
	static uint8_t* video_memory = (uint8_t*)0xB8000;
	for(; *chars != 0; chars++) {
		if(*chars == '\n') {
			continue;
		}

		*video_memory++ = *chars;
		*video_memory++ = 0x1F;
	}
}

static void stacktrace() {
	intptr_t addresses[10];
	int read = walk_stack(addresses, 10);

	for(int i = 0; i < read; i++) {
		panic_printf("#%d\t0x%x\n", i, addresses[i]);
	}
}

void __attribute__((optimize("O0"))) panic(char* error) {
	interrupts_disable();

	bruteforce_print("Early Kernel Panic: ");
	bruteforce_print(error);
	bruteforce_print(" -- If you can see only this message, but not the full kernel "
		"panic debug information, either the console framework / display driver "
		"failed or the kernel panic occured in early startup before the "
		"initialization of the needed drivers.");

	panic_printf("\n%%Kernel Panic: %s%%\n", 0x04, error);

	uint32_t ticknum = pit_getTickNum();
	panic_printf("Last PIT tick: %d (rate %d, uptime: %d seconds)\n",
		ticknum,
		PIT_RATE,
		ticknum / PIT_RATE);

	task_t* task = scheduler_get_current();
	if(task) {
		panic_printf("Running task: %d <%s>", task->pid, task->name);

		uint32_t task_offset = task->state->eip - task->entry;
		if(task_offset >= 0) {
			panic_printf("+%x", task_offset);
		}

		panic_printf("\n");
	} else {
		panic_printf("Running task: [No task running]\n");
	}

	if(!vfs_last_read_attempt[0]) {
		strcpy(vfs_last_read_attempt, "No file system read attempts.");
	}

	panic_printf("Last VFS read attempt: %s\n", vfs_last_read_attempt);
	panic_printf("Active paging context: %s\n\n", vmem_get_name(vmem_currentContext));

	panic_printf("Call trace:\n");
	stacktrace();
	freeze();
}

void panic_init(multiboot_info_t* multiboot_info) {}
