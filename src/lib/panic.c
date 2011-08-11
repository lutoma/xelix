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
#include <interrupts/interface.h>
#include <hw/cpu.h>


// Panic. Use the panic() macro that inserts the line.
static void panicHandler(cpu_state_t* regs)
{
	// Disable interrupts.
	interrupts_disable();
	console_clear(NULL);

	printf("%%Kernel Panic!%%\n\n", 0x04);
	

	printf("Technical information:\n\n");
	printf("Last PIT ticknum: %d\n", pit_getTickNum());
	
	task_t* task = scheduler_getCurrentTask();
	
	if(task != NULL)
		printf("Last task PID: %d\n\n", task->pid);
	else
		printf("Last task PID: (null)\n\n");


	//Sleep forever
	freeze();
}

void panic_init()
{
	interrupts_registerHandler(0x03, panicHandler);
}
