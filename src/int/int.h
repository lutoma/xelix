#pragma once

/* Copyright © 2011-2019 Lukas Martini
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
#include <stdbool.h>

#ifdef __i386__
	#define IRQ(n) (n + 0x20)
	#define EFLAGS_IF 0x200
#else
	// First 0x10 are reserved by us for exception types
	#define IRQ(n) (n + 0x10)

	#define ARM_INT_TYPE_SVC 1
	#define ARM_INT_TYPE_IRQ 2
	#define ARM_INT_TYPE_FIQ 3
#endif

/* Interrupt stack frame */
typedef struct {
#ifdef __i386__
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
#else /* ARM */
   // user mode r13 & r14
   uint32_t usr_r13;
   uint32_t usr_r14;

/*
   // CPSR before IRQ and supervisor mode SPSR
   uint32_t cpsr;
   uint32_t svc_spsr;

   // supervisor mode r13 & r14
   uint32_t svc_r13;
   uint32_t svc_r14;
*/

   uint32_t r0, r1, r2, r3, r4, r5, r6, r7;
   uint32_t r8, r9, r10, r11, r12, r15;
#endif
} __attribute__((__packed__)) isf_t;

#ifdef __i386__
typedef struct {
	/* Pushed by the processor automatically. This is what the processor
	 * expects to be in the kernel stack when doing an iret to ring 3.
	 */
	void* entry;
	uint32_t cs;
	uint32_t eflags;
	uint32_t user_esp;
	uint32_t ss;
} __attribute__((__packed__)) iret_t;
#endif

typedef void (*interrupt_handler_t)(isf_t*);
struct interrupt_reg {
	interrupt_handler_t handler;
	bool can_reent;
};

struct interrupt_reg interrupt_handlers[256];

static inline void interrupts_register(uint8_t n, interrupt_handler_t handler, bool can_reent) {
	interrupt_handlers[n].handler = handler;
	interrupt_handlers[n].can_reent = can_reent;
	log(LOG_INFO, "interrupts: Registered handler for 0x%x.\n", n);
}

static inline void interrupts_bulk_register(uint8_t start, uint8_t end, interrupt_handler_t handler, bool can_reent) {
		for(uint8_t i = start; i <= end; i++) {
			interrupt_handlers[i].handler = handler;
			interrupt_handlers[i].can_reent = can_reent;
		}

		log(LOG_INFO, "interrupts: Registered handlers for 0x%x - 0x%x.\n", start, end);
}

static inline void dump_isf(uint32_t level, isf_t* state) {
	log(level, "isf_t at 0x%x:\n", state);
	#ifdef __i386__
	log(level, "  EAX=0x%-10x EBX=0x%-10x ECX=0x%-10x EDX=0x%-10x\n", state->eax, state->ebx, state->ecx, state->edx);
	log(level, "  ESI=0x%-10x EDI=0x%-10x EBP=0x%-10x ESP=0x%-10x\n", state->esi, state->edi, state->ebp, state->esp);
	log(level, "  DS=0x%-10x  CR3=0x%-10x\n", state->ds, state->cr3);
	#else
	log(level, "  R0=0x%-10x R1=0x%-10x R2=0x%-10x R3=0x%-10x\n", state->r0, state->r1, state->r2, state->r3);
	log(level, "  R4=0x%-10x R5=0x%-10x R6=0x%-10x R7=0x%-10x\n", state->r4, state->r5, state->r6, state->r7);
	log(level, "  R8=0x%-10x R9=0x%-10x R10=0x%-10x R11=0x%-10x\n", state->r8, state->r9, state->r10, state->r11);
	#endif
}

void interrupts_init();
