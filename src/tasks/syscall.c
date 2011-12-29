/* syscall.c: Syscalls
 * Copyright © 2011 Lukas Martini, Fritz Grimpen
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

#include "syscall.h"
#include <lib/generic.h>
#include <interrupts/interface.h>
#include <lib/log.h>
#include <tasks/scheduler.h>
#include <lib/print.h>

#include "syscalls.h"

static void intHandler(cpu_state_t* regs)
{
	struct syscall syscall;
	syscall.num = regs->eax;
	syscall.params[0] = regs->ebx;
	syscall.params[1] = regs->ecx;
	syscall.params[2] = regs->edx;
	syscall.params[3] = regs->esi;
	syscall.params[4] = regs->edi;
	syscall.params[5] = (int)regs->ebp;

	syscall_t call = syscall_table[syscall.num];
	if (syscall.num >= sizeof(syscall_table) / sizeof(syscall_t) || call == NULL)
	{
		log(LOG_INFO, "syscall: Invalid syscall %d\n", syscall.num);
		syscall.num = -1;
		return;
	}

	regs->eax = call(syscall);
}

void syscall_init()
{
	interrupts_registerHandler(SYSCALL_INTERRUPT, intHandler);
}
