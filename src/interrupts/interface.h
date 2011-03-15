#pragma once

/* Copyright © 2010 Christoph Sünderhauf
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

#include <lib/generic.h>

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

typedef struct {
	uint32 ds;                  // Data segment selector
	uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
	uint32 int_no, err_code;    // Interrupt number and error code (if applicable)
	uint32 eip, cs, eflags, useresp, ss; // Pushed by the processor automatically. // This is what the processor expects to be in the stack when doing an iret. useresp and ss are only used when returning to another privilege level
} registers_t;

typedef void (*interruptHandler_t)(registers_t);

void interrupts_registerHandler(uint8 n, interruptHandler_t handler);
void interrupts_init();
