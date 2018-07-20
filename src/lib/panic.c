/* panic.c: Handle kernel panics
 * Copyright © 2011 Benjamin Richter
 * Copyright © 2011-2016 Lukas Martini
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
#include <console/interface.h>
#include <hw/interrupts.h>
#include <hw/cpu.h>
#include <hw/pit.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <lib/string.h>
#include <memory/vmem.h>

static inline void panic_printf(const char *fmt, ...) {
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

static void dump_registers(cpu_state_t* regs) {
	panic_printf("CPU State:\n");
	panic_printf("EAX=0x%x\tEBX=0x%x\tECX=0x%x\tEDX=0x%x\n",
		regs->eax, regs->ebx, regs->ecx, regs->edx);
	panic_printf("ESI=0x%x\tEDI=0x%x\tESP=0x%x\tEBP=0x%x\n",
		regs->esi, regs->edi, regs->esp, regs->ebp);
	panic_printf("DS=0x%x\tSS=0x%x\tCS=0x%x\tEFLAGS=0x%x\n",
		regs->ds, regs->ss, regs->cs, regs->eflags);
	panic_printf("ESP=0x%x\tEBP=0x%x\tEIP=0x%x\n",
		regs->esp, regs->ebp, regs->eip);
}

static  __attribute__((optimize("O0")))  void panic_handler(cpu_state_t* regs)
{
	bruteforce_print("Early Kernel Panic: ");
	bruteforce_print(*((char**)PANIC_INFOMEM));
	bruteforce_print(" -- If you can see only this message, but not the full kernel "
		"panic debug information, either the console framework / display driver "
		"failed or the kernel panic occured in early startup before the "
		"initialization of the needed drivers.");

	panic_printf("\n%%Kernel Panic: %s%%\n", 0x04, *((char**)PANIC_INFOMEM));

	uint32_t ticknum = pit_getTickNum();
	panic_printf("Last PIT ticknum: %d (tickrate %d, approx. uptime: %d seconds)\n",
		ticknum,
		PIT_RATE,
		ticknum / PIT_RATE);

	task_t* task = scheduler_get_current();

	if(task != NULL) {
		panic_printf("Running task: %d <%s>", task->pid, task->name);

		uint32_t task_offset = task->state->eip - task->entry;
		if(task_offset >= 0) {
			panic_printf("+%x", task_offset);
		}

		panic_printf("\n");
	} else
		panic_printf("Running task: [No task running]\n");

	if(vfs_last_read_attempt[0] == '\0') {
		strncpy(vfs_last_read_attempt, "No file system read attempts.", 512);
	}

	panic_printf("Last VFS read attempt: %s\n", vfs_last_read_attempt);
	panic_printf("Active paging context: %s\n\n", vmem_get_name(vmem_currentContext));

	dump_registers(regs);
	freeze();
}

void panic_init()
{
	interrupts_registerHandler(0x30, panic_handler);
}
