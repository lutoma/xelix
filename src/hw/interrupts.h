#pragma once

/* Copyright © 2010 Christoph Sünderhauf
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

#include <log.h>

#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

#define interrupts_disable() asm volatile("cli")
#define interrupts_enable() asm volatile("sti")

#define EFLAGS_IF 0x200

/* Interrupt stack frame */
typedef struct {
	uint32_t cr3;
	uint32_t ds;

	uint32_t edi;
	uint32_t esi;
	uint8_t* ebp;
	void* _unused;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	void* esp;

	/* Pushed by the processor automatically. This is what the processor
	 * expects to be in the stack when doing an iret.
	 */
/*	void* eip;
	uint32_t cs;
	uint32_t eflags;*/
} __attribute__((__packed__)) isf_t;

typedef void (*interrupt_handler_t)(isf_t*);
interrupt_handler_t interrupt_handlers[256];

static inline void interrupts_register(uint8_t n, interrupt_handler_t handler) {
	interrupt_handlers[n] = handler;
	log(LOG_INFO, "interrupts: Registered handler for 0x%x.\n", n);
}

static inline void interrupts_bulk_register(uint8_t start, uint8_t end, interrupt_handler_t handler) {
		for(uint8_t i = start; i <= end; i++)
			interrupt_handlers[i] = handler;

		log(LOG_INFO, "interrupts: Registered handlers for 0x%x - 0x%x.\n", start, end);
}

static inline void dump_isf(uint32_t level, isf_t* state) {
	log(level, "isf_t at 0x%x:\n", state);
	log(level, "  EAX=0x%-10x EBX=0x%-10x ECX=0x%-10x EDX=0x%-10x\n", state->eax, state->ebx, state->ecx, state->edx);
	log(level, "  ESI=0x%-10x EDI=0x%-10x EBP=0x%-10x ESP=0x%-10x\n", state->esi, state->edi, state->ebp, state->esp);
	//log(level, "  EIP=0x%-10x CR2=0x%-10x CR3=0x%-10x EFLAGS=0x%-10x\n", state->eip, state->cr2, state->cr3, state->eflags);
}

void interrupts_init();
