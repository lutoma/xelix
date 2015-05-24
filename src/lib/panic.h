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
#include <hw/interrupts.h>
#include <hw/pit.h>
#include <tasks/scheduler.h>

#define PANIC_INFOMEM 0x100
#define panic(error) do { \
	interrupts_disable();  \
	*((char**)PANIC_INFOMEM) = (char*)(error); \
	asm("int 0x30; cli;"); \
} while(0)

#define assert(b) if(!(b)) panic("Assertion \"" #b "\" failed.")

void dumpCpuState(cpu_state_t* regs);
void panic_init();
