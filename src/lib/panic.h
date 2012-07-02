#pragma once

/* Copyright © 2011 Lukas Martini
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include "generic.h"
#include "print.h"
#include <interrupts/interface.h>
#include <hw/pit.h>
#include <tasks/scheduler.h>

/* Maybe we should use preallocated memory somewhere else to circumvent the
 * possibility of overwriting possibly valuable data or the kernel itself
 * when a kernel panic occurs. Then again, when you've got a kernel panic,
 * you're fucked anyways, so it shouldn't matter.
 */
#define PANIC_INFOMEM 0x100
#define PANIC_STACKTRACEMEM(i) (0x200 + sizeof(void*) * i)

// No one said it's going to be beautiful.
#define panic(error) do {																	\
	interrupts_disable(); 																	\
	*((char**)PANIC_INFOMEM) = (char*)(error);												\
																							\
	for(int i = 0; i < 15; i++)																\
		*((void**)PANIC_STACKTRACEMEM(i)) = 												\
			__builtin_extract_return_address(__builtin_return_address(i));					\
																							\
	asm("int 0x30");																		\
} while(0)

#define assert(b) if(!(b)) panic("Assertion \"" #b "\" failed.")

void dumpCpuState(cpu_state_t* regs);
void panic_init();