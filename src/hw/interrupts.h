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

#include <generic.h>
#include <log.h>
#include <hw/cpu.h>

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

typedef cpu_state_t* (*interrupt_handler_t)(cpu_state_t*);
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

void interrupts_init();
