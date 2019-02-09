/* syscall.c: Syscall handling
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include "syscall.h"
#include <hw/interrupts.h>
#include <log.h>
#include <tasks/scheduler.h>
#include <print.h>
#include <panic.h>
#include <errno.h>
#include <memory/vmem.h>

#include "syscalls.h"

static inline int handle_arg_flags(struct syscall* syscall, int num, uint8_t flags) {
	if(flags & SYSCALL_ARG_RESOLVE) {
		if(flags & SYSCALL_ARG_RESOLVE_NULL_OK && !syscall->params[num]) {
			return 0;
		}

		syscall->params[num] = vmem_translate(syscall->task->memory_context,
			syscall->params[num], false);

		if(!syscall->params[num]) {
			return -1;
		}
	}

	return 0;
}

static void int_handler(isf_t* regs)
{
	task_t* task = scheduler_get_current();
	if(!task) {
		log(LOG_WARN, "syscall: Got interrupt, but there is no current task.\n");
		return;
	}

	task->task_state = TASK_STATE_SYSCALL;

	struct syscall syscall;
	syscall.num = regs->eax;
	syscall.params[0] = regs->ebx;
	syscall.params[1] = regs->ecx;
	syscall.params[2] = regs->edx;
	syscall.state = regs;
	syscall.task = task;

	struct syscall_definition def = syscall_table[syscall.num];
	if (syscall.num >= sizeof(syscall_table) / sizeof(struct syscall_definition) || def.handler == NULL) {
		log(LOG_WARN, "syscall: Invalid syscall %d\n", syscall.num);
		regs->eax = -1;
		regs->ebx = EINVAL;
		return;
	}

#ifdef SYSCALL_DEBUG
	task_t* cur = scheduler_get_current();
	log(LOG_DEBUG, "PID %d <%s>: %s(0x%x 0x%x 0x%x)\n",
		cur->pid, cur->name,
		def.name,
		regs->ebx,
		regs->ecx,
		regs->edx);
#endif

	if(handle_arg_flags(&syscall, 0, def.arg0) == -1 ||
		handle_arg_flags(&syscall, 1, def.arg1) == -1 ||
		handle_arg_flags(&syscall, 2, def.arg2) == -1) {

		regs->eax = -1;
		regs->ebx = EINVAL;
		return;
	}

	task->syscall_errno = 0;
	regs->eax = def.handler(syscall);
	regs->ebx = task->syscall_errno;

	// Only change state back if it hasn't alreay been modified
	if(task->task_state == TASK_STATE_SYSCALL) {
		task->task_state = TASK_STATE_RUNNING;
	}

#ifdef SYSCALL_DEBUG
	log(LOG_DEBUG, "Result: 0x%x, errno: %d\n", regs->eax, regs->ebx);
#endif
}

void syscall_init() {
	interrupts_register(SYSCALL_INTERRUPT, int_handler, true);
}
