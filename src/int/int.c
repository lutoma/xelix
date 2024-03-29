/* int.c: Interrupt dispatching
 * Copyright © 2011-2020 Lukas Martini
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
#include <mem/paging.h>
#include <mem/i386-gdt.h>

#define debug(args...) log(LOG_DEBUG, "interrupts: " args)

isf_t* __fastcall int_dispatch(uint32_t intr, isf_t* state);

struct interrupt_reg int_handlers[512][10];
uint8_t sse_state[512] __aligned(16) UL_VISIBLE("bss");
uint8_t* int_sse_target UL_VISIBLE("data") = sse_state;

// Called by architecture-specific assembly handlers
isf_t* __fastcall int_dispatch(uint32_t intr, isf_t* state) {
	memcpy(state->sse_state, sse_state, 512);
	scheduler_store_isf(state);

	struct interrupt_reg* reg = int_handlers[intr];
	task_t* task = scheduler_get_current();

	#ifdef CONFIG_INTERRUPTS_DEBUG
	debug("state before:\n");
	dump_isf(LOG_DEBUG, state);
	#endif

	int_disable();

	for(int i = 0; i < 10; i++) {
		if(!reg[i].handler) {
			break;
		}

		if(reg[i].can_reent) {
			int_enable();
		}

		reg[i].handler((task_t*)task, state, intr);
	}

	// Run scheduler every tick, or when task yields
	if(intr == IRQ(0) || intr == 0x31 || (task && task->interrupt_yield)) {
		if((task && task->interrupt_yield)) {
			task->interrupt_yield = false;
		}

		isf_t* new_state = scheduler_select(state);
		if(new_state) {
			#ifdef CONFIG_INTERRUPTS_DEBUG
			debug("state after (task selection):\n");
			dump_isf(LOG_DEBUG, new_state);
			#endif

			return new_state;
		}
	}

	#ifdef CONFIG_INTERRUPTS_DEBUG
	debug("state after:\n");
	dump_isf(LOG_DEBUG, state);
	#endif

	memcpy(sse_state, state->sse_state, 512);
	return state;
}

void int_init(void) {
	idt_init();
	bzero(int_handlers, sizeof(int_handlers));
	int_enable();
}
