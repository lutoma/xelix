#pragma once

/* Copyright © 2011-2020 Lukas Martini
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

	#define int_disable() asm volatile("cli")
	#define int_enable() asm volatile("sti")
#endif

struct task;

/* Interrupt stack frame */
typedef struct {
	uint8_t sse_state[512];
	uint32_t cr3;
	void* cr2;
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

	uint32_t err_code;
} __attribute__((__packed__)) isf_t;

/* Pushed by the processor automatically. This is what the processor
 * expects to be in the kernel stack when doing an iret to ring 3.
 */
typedef struct {
	void* eip;
	uint32_t cs;
	uint32_t eflags;
	void* user_esp;
	uint32_t ss;
} __attribute__((__packed__)) iret_t;

typedef void (*interrupt_handler_t)(struct task* task, isf_t* state, int num);
struct interrupt_reg {
	interrupt_handler_t handler;
	bool can_reent;
};

// Can't use kmalloc here as this is used during early boot. 10 should be plenty
extern struct interrupt_reg int_handlers[512][10];

static inline void int_register(int n, interrupt_handler_t handler, bool can_reent) {
	struct interrupt_reg* reg = int_handlers[n];

	for(int i = 0; i < 10; i++) {
		if(reg[i].handler) {
			continue;
		}

		reg[i].handler = handler;
		reg[i].can_reent = can_reent;
		return;
	}

	log(LOG_ERR, "int: Could not register handler for %d, too many handlers\n", n);
}

static inline void int_register_bulk(int start, int end, interrupt_handler_t handler, bool can_reent) {
		for(int i = start; i <= end; i++) {
			int_register(i, handler, can_reent);
		}
}

static inline void dump_isf(uint32_t level, isf_t* state) {
	log(level, "isf_t at 0x%x:\n", state);
	log(level, "  EAX=0x%-10x EBX=0x%-10x ECX=0x%-10x EDX=0x%-10x\n", state->eax, state->ebx, state->ecx, state->edx);
	log(level, "  ESI=0x%-10x EDI=0x%-10x EBP=0x%-10x ESP=0x%-10x\n", state->esi, state->edi, state->ebp, state->esp);
}

void int_init(void);
