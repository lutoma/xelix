/* interrupts.c: Interrupt dispatching
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

#include <hw/interrupts.h>
#include <string.h>
#include <portio.h>
#include <hw/idt.h>
#include <tasks/scheduler.h>
#include <mem/vmem.h>
#include <mem/paging.h>
#include <mem/gdt.h>

#ifdef ENABLE_PICOTCP
#include <net/net.h>
#endif

#define debug(args...) log(LOG_DEBUG, "interrupts: " args)

#ifdef __arm__
extern uint32_t arm_exception_vectors[];
#endif

static void dispatch(uint32_t intr, isf_t* regs) {
	struct interrupt_reg reg = interrupt_handlers[intr];

	if(reg.handler) {
		if(reg.can_reent) {
			interrupts_enable();
		}
		reg.handler(regs);
	}
}

#ifdef __arm__
static void arm_dispatch(uint32_t type, isf_t* regs) {
	if(type != ARM_INT_TYPE_IRQ) {
		dispatch(type, regs);
		return;
	}

	// Retrieve CPU no from bits 0 & 1 of MPIDR
	uint32_t cpu;
	asm volatile("mrc p15,0,%0,c0,c0,5" : "=r" (cpu));
	cpu &= 3;

	uint32_t pending = bcm2836_mmio_read((0x60 + 4 * cpu));
	uint32_t irq;

	// There may be multiple pending IRQs
	while ((irq = __builtin_ffs(pending))) {
		irq -= 1;
		uint32_t _irq = IRQ(cpu * 32 + irq);
		dispatch(_irq, regs);
		pending &= ~(1 << irq);
	}
}
#endif

// Called from i386-interrupts.asm and arm-interrupts.S
isf_t* __fastcall interrupts_callback(uint32_t intr, isf_t* regs) {
	#ifdef INTERRUPTS_DEBUG
	debug("state before:\n");
	dump_isf(LOG_DEBUG, regs);
	#endif

	#ifdef __i386__
	dispatch(intr, regs);
	#else
	arm_dispatch(intr, regs);
	#endif

	#ifdef ENABLE_PICOTCP
	if(intr == IRQ(0)) {
		net_tick();
	}
	#endif

	task_t* task = scheduler_get_current();
	// Run scheduler every 100th tick, or when task yields
	if((intr == IRQ(0) && !(timer_get_tick() % 100)) || (task && task->interrupt_yield)) {
		if((task && task->interrupt_yield)) {
			task->interrupt_yield = false;
		}

		task_t* new_task = scheduler_select(regs);
		if(new_task && new_task->state) {
			#ifdef INTERRUPTS_DEBUG
			debug("state after (task selection):\n");
			dump_isf(LOG_DEBUG, new_task->state);
			#endif

			#ifdef __i386__
				gdt_set_tss(new_task->kernel_stack + PAGE_SIZE);
			#endif
			return new_task->state;
		}
	}

	#ifdef INTERRUPTS_DEBUG
	debug("state after:\n");
	dump_isf(LOG_DEBUG, regs);
	#endif
	return regs;
}

void interrupts_init() {
	#ifdef __i386__
	idt_init();
	#else
	serial_printf("interrupts: Setting VBAR to %#x\n", arm_exception_vectors);
	asm volatile("mcr p15, 0, %0, c12, c0, 0" :: "r" (arm_exception_vectors));
	asm volatile("mcr p15, 4, %0, c12, c0, 0" :: "r" (arm_exception_vectors));
	#endif

	bzero(interrupt_handlers, sizeof(interrupt_handlers));
	interrupts_enable();
}
