/* panic.c: Description of what this file does
 * Copyright © 2011 Lukas Martini, Benjamin Richter
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
#include <interrupts/interface.h>
#include <hw/cpu.h>

void dumpCpuState(cpu_state_t* regs) {
	printf("CPU State:\n");
	printf("EAX=0x%x\tEBX=0x%x\tECX=0x%x\tEDX=0x%x\n",
		regs->eax, regs->ebx, regs->ecx, regs->edx);
	printf("ESI=0x%x\tEDI=0x%x\tESP=0x%x\tEBP=0x%x\n",
		regs->esi, regs->edi, regs->esp, regs->ebp);
	printf("DS=0x%x\tSS=0x%x\tCS=0x%x\tEFLAGS=0x%x\n",
		regs->ds, regs->ss, regs->cs, regs->eflags);

	printf("\n");
	printf("Return Addresses:\n");
	uint8_t* bp = regs->ebp;
	do {
		printf("* 0x%x\n", *(bp + 2));
		bp = (uint8_t*)*((uint32_t *)bp + 1);
	} while (bp);
}

static void panicHandler(cpu_state_t* regs)
{
	printf("%%Kernel Panic: %s%%\n\n", 0x04, *((char**)PANIC_INFOMEM));

	printf("Technical information:\n\n");
	printf("Last PIT ticknum: %d\n", pit_getTickNum());
	
	task_t* task = scheduler_getCurrentTask();
	
	if(task != NULL)
		printf("Running task: %d\n\n", task->pid);
	else
		printf("Running task: [No task running]\n\n");

	dumpCpuState(regs);

	freeze();
}

void panic_init()
{
	interrupts_registerHandler(0x30, panicHandler);
}
