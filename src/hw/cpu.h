#pragma once

/* Copyright © 2010-2016 Lukas Martini
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


typedef struct {
	// Data segment selector
	uint32_t ds;

	uint32_t edi;
	uint32_t esi;
	uint8_t* ebp;
	uint32_t user_esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	// Page fault address. Will always be 0 except while in a page fault interrupt
	uint32_t cr2;

	// Interrupt number and error code (if applicable)
	uint32_t interrupt;
	uint32_t errCode;

	/* Pushed by the processor automatically. This is what the processor
	 * expects to be in the stack when doing an iret. useresp and ss are
	 * only used when returning to another privilege level
	 */
	void* eip;
	uint32_t cs;
	uint32_t eflags;
	void* esp;
	uint32_t ss;
} __attribute__((__packed__)) cpu_state_t;


void cpu_init_fault_handlers();

static inline void cpu_init() {
	cpu_init_fault_handlers();
}

