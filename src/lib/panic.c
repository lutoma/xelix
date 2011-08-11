/* panic.c: Description of what this file does
 * Copyright © 2011 Lukas Martini
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

#define dumpRegister(reg) \
	asm("mov %0, " reg : "=m" (temp)); \
	printf("%s = 0x%x%s", reg, temp, (num % 4) ? "   " : "\n"); \
	num++

// Panic. Use the panic() macro that inserts the line.
void panic_raw(char *file, uint32_t line, const char *reason, ...)
{
	// Disable interrupts.
	interrupts_disable();
	console_clear(NULL);

	printf("%%Kernel Panic!%%\n\n", 0x04);
	
	vprintf(reason, (void**)(&reason) + 1);	
	printf("\n\n");

	printf("Technical information:\n\n");
	printf("Caller: %s, line %d.\n", file, line);
	printf("Last PIT ticknum: %d\n", pit_getTickNum());
	
	task_t* task = scheduler_getCurrentTask();
	
	if(task != NULL)
		printf("Last task PID: %d\n\n", task->pid);
	else
		printf("Last task PID: (null)\n\n");


	uint32_t temp, num = 1;

	// EAX, EBX, ECX, EDX, EDI, ESI, EBP, ESP, EIP, EFLAGS

	printf("Register contents:\n\n");
	dumpRegister("eax");
	dumpRegister("ebx");
	dumpRegister("ecx");
	dumpRegister("edx");	
	dumpRegister("edi");	
	dumpRegister("esi");	
	dumpRegister("ebp");	
	dumpRegister("esp");

	// Get eflags
	asm("pushf; pop eax;" ::: "eax");
	asm("mov %0, eax" : "=m" (temp));
	printf("eflags = 0x%x%s", temp, (num % 4) ? "   " : "\n");
	num++;

	//Sleep forever
	freeze();
}
