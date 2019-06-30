/* interrupts.c: Initialization of and interface to interrupts.
 * Copyright © 2011-2019 Lukas Martini
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

#include <int/int.h>
#include <string.h>
#include <int/i386-idt.h>
#include <tasks/scheduler.h>
#include <mem/vmem.h>
#include <mem/paging.h>
#include <mem/i386-gdt.h>
#include <net/net.h>

#define debug(args...) log(LOG_DEBUG, "interrupts: " args)

// Called by i386-interrupts.asm
isf_t* __fastcall interrupts_callback(uint32_t intr, isf_t* state) {
	struct interrupt_reg reg = interrupt_handlers[intr];
	volatile task_t* task = scheduler_get_current();

	#ifdef INTERRUPTS_DEBUG
	debug("state before:\n");
	dump_isf(LOG_DEBUG, state);
	#endif

	if(reg.handler) {
		if(reg.can_reent) {
			interrupts_enable();
		}
		reg.handler((task_t*)task, state, intr);
	}

	#ifdef ENABLE_PICOTCP
	if(intr == IRQ(0)) {
		net_tick();
	}
	#endif

	// Run scheduler every 100th tick, or when task yields
	if((intr == IRQ(0) && !(timer_get_tick() % 100)) || (task && task->interrupt_yield)) {
		if((task && task->interrupt_yield)) {
			task->interrupt_yield = false;
		}

		task_t* new_task = scheduler_select(state);
		if(new_task && new_task->state) {
			#ifdef INTERRUPTS_DEBUG
			debug("state after (task selection):\n");
			dump_isf(LOG_DEBUG, new_task->state);
			#endif

			gdt_set_tss(new_task->kernel_stack + PAGE_SIZE);
			return new_task->state;
		}
	}

	#ifdef INTERRUPTS_DEBUG
	debug("state after:\n");
	dump_isf(LOG_DEBUG, state);
	#endif
	return state;
}

void interrupts_init() {
	idt_init();
	bzero(interrupt_handlers, sizeof(interrupt_handlers));
	interrupts_enable();
}
