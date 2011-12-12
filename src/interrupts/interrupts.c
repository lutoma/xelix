/* interrupts.c: Initialization of and interface to interrupts.
 * Copyright © 2010 Christoph Sünderhauf
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "interface.h"
#include <lib/log.h>
#include <lib/generic.h>
#include <arch/interrupts.h>
#include <tasks/scheduler.h>
#include <memory/vmem.h>
#include <memory/paging.h>

interruptHandler_t interruptHandlers[256];

/* This one get's called from the architecture-specific interrupt
 * handlers, which do fiddling like EOIs (i386).
 */
cpu_state_t* interrupts_callback(cpu_state_t* regs)
{
	vmem_processContext = vmem_currentContext;
	paging_apply(vmem_kernelContext);
	interruptHandler_t handler = interruptHandlers[regs->interrupt];

	if(handler != NULL)
		handler(regs);
	
	if((regs->interrupt == IRQ0 || regs->interrupt == 0x31) && scheduler_state) // PIT
	{
		
		task_t* nowTask = scheduler_select(regs);
		if((int)nowTask != NULL && (int)nowTask->state != NULL)
		{
			paging_apply(nowTask->memory_context);
			return nowTask->state;
		}
	}

	paging_apply(vmem_processContext);
	return regs;
}

void interrupts_registerHandler(uint8_t n, interruptHandler_t handler)
{
	interruptHandlers[n] = handler;
	
	if(n > 31)
		log(LOG_INFO, "interrupts: Registered interrupt handler for IRQ %d.\n", n - 32);
	else
		log(LOG_INFO, "interrupts: Registered interrupt handler for %d.\n", n);
}

void interrupts_bulkRegisterHandler(uint8_t start, uint8_t end, interruptHandler_t handler)
{
		if(start >= end)
		{
			log(LOG_WARN, "interrupts: Warning: Attempt to bulk register interrupt handlers with start >= end.\n");
			return;
		}
		
		int32_t i;
		for(i = start; i <= end; i++)
			interruptHandlers[i] = handler;

		if(start > 31)
			log(LOG_INFO, "interrupts: Registered interrupt handler for IRQs %d -  %d.\n", start - 32, end - 32);
		else
			log(LOG_INFO, "interrupts: Registered interrupt handlers for %d - %d.\n", start, end);
}

void interrupts_init()
{
	arch_interrupts_init();

	// set all interruptHandlers to NULL.
	memset(interruptHandlers, NULL, 256 * sizeof(interruptHandler_t));
	interrupts_enable();
	
	log(LOG_INFO, "interrupts: Initialized\n");
}
