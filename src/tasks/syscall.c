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
#include <lib/print.h>
#include <tasks/scheduler.h>

typedef int (*syscall_t)(cpu_state_t *);

static int syscall_exit(cpu_state_t *regs)
{
	scheduler_terminateCurrentTask();
	return 0;
}

static int syscall_print(cpu_state_t *regs)
{
	return print((char *)regs->ecx);
}

static syscall_t jump_table[] = {
	syscall_print,   /* 0 */
	syscall_exit,    /* 1 */
};

static void intHandler(cpu_state_t* regs)
{
	syscall_t call = jump_table[regs->eax];
	if (regs->eax >= sizeof(jump_table) / sizeof(syscall_t) || call == NULL)
	{
		log(LOG_INFO, "syscall: Invalid syscall %d\n", regs->eax);
		regs->eax = -1;
		return;
	}

	regs->eax = call(regs);
}

void syscall_init()
{
	interrupts_registerHandler(SYSCALL_INTERRUPT, intHandler);
}
