#pragma once

/* Copyright © 2011-2016 Lukas Martini
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

#include <hw/interrupts.h>
#include <tasks/scheduler.h>

#define SYSCALL_INTERRUPT 0x80

#define SYSCALL_HANDLER(name) uint32_t sys_ ## name (struct syscall syscall)

#define _SYSC_RESOLVE(par, reverse) {									\
	par = (typeof(par))task_resolve_address((intptr_t)par, reverse);	\
	if(!par) {															\
		return -1;														\
	}																	\
}

#define SYSCALL_SAFE_RESOLVE_PARAM(par) _SYSC_RESOLVE(syscall.params[par], false)
#define SYSCALL_SAFE_RESOLVE(par) _SYSC_RESOLVE(par, false)
#define SYSCALL_SAFE_REVERSE_RESOLVE(par) _SYSC_RESOLVE(par, true)

struct syscall {
	int num;
	int params[3];
	cpu_state_t* state;
	task_t* task;
};

typedef uint32_t (*syscall_t)(struct syscall);
intptr_t task_resolve_address(intptr_t raddress, bool reverse);
void syscall_init();
