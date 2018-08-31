/* interrupts.c: Initialization of and interface to interrupts.
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <hw/interrupts.h>
#include <string.h>
#include <hw/idt.h>
#include <tasks/scheduler.h>
#include <memory/vmem.h>
#include <memory/paging.h>

/* This one get's called from the architecture-specific interrupt
 * handlers, which do fiddling like EOIs (i386).
 */
cpu_state_t* __attribute__((fastcall)) interrupts_callback(cpu_state_t* regs) {
	struct vmem_context* original_context = vmem_currentContext;

	paging_apply(vmem_kernelContext);
	interrupt_handler_t handler = interrupt_handlers[regs->interrupt];

	if(handler != NULL)
		handler(regs);

	/* Timer interrupt
	 * FIXME Should get a normal interrupt handler like everything else
	 */
	if((regs->interrupt == IRQ0 || regs->interrupt == 0x31) && scheduler_state != SCHEDULER_OFF)
	{
		task_t* new_task = scheduler_select(regs);

		if(new_task && new_task->state && new_task->memory_context)
		{
			paging_apply(new_task->memory_context);
			return new_task->state;
		}
	}

	paging_apply(original_context);
	return regs;
}

void interrupts_init() {
	idt_init();
	bzero(interrupt_handlers, sizeof(interrupt_handlers));
	interrupts_enable();
}
