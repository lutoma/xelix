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
#include <lib/string.h>
#include <console/interface.h>

typedef int (*syscall_t)(cpu_state_t *);

static int syscall_exit(cpu_state_t *regs)
{
	scheduler_getCurrentTask()->task_state = TASK_STATE_TERMINATED;
	return 0;
}

static int syscall_write(cpu_state_t *regs)
{
	if (regs->ebx == 1 || regs->ebx == 2)
	{
		if (regs->ebx == 2)
			console_write2(NULL, "\e[31m");

		int retval = console_write(NULL, (char*)regs->ecx, regs->edx);

		if (regs->ebx == 2)
			console_write2(NULL, "\e[39m");

		return retval;
	}

	return -1;
}

static int syscall_getpid(cpu_state_t *regs)
{
	return scheduler_getCurrentTask()->pid;
}

static int syscall_getppid(cpu_state_t *regs)
{
	return scheduler_getCurrentTask()->parent;
}

static int syscall_print(cpu_state_t *regs)
{
	return console_write2(NULL, (char*)regs->ebx);
}

static syscall_t jump_table[] = {
	syscall_print, // 0
  syscall_exit, // 1
	NULL,
	NULL,
	syscall_write, // 4
	syscall_getppid,
	syscall_getpid
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
