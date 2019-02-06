/* signal.c: Task signals
 * Copyright © 2019 Lukas Martini
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

#include <tasks/signal.h>
#include <tasks/task.h>
#include <errno.h>

extern void __attribute__((fastcall)) task_sigjmp_crt0(void* addr);

int __attribute__((optimize("O0"))) task_signal(task_t* task, int sig, isf_t* state) {
	if(sig > 32) {
		sc_errno = EINVAL;
		return -1;
	}

	if(sig == SIGKILL || sig == SIGSTOP) {
		task->task_state = (sig == SIGKILL) ? TASK_STATE_TERMINATED : TASK_STATE_STOPPED;
		task->interrupt_yield = true;
		return 0;
	}

	struct sigaction sa = task->signal_handlers[sig];
	if((uint32_t)sa.sa_handler == SIG_IGN) {
		return 0;
	}

	if(sa.sa_handler && (uint32_t)sa.sa_handler != SIG_DFL) {
		iret_t* iret = task->kernel_stack + PAGE_SIZE - sizeof(iret_t);

		iret->user_esp -= 11 * sizeof(uint32_t);
		uint32_t* user_stack = vmem_translate(task->memory_context, iret->user_esp, false);
		*user_stack = sa.sa_handler;
		*(user_stack + 1) = sig;

		// popa registers
		*(user_stack + 2) = state->edi;
		*(user_stack + 3) = state->esi;
		*(user_stack + 4) = state->ebp;
		*(user_stack + 5) = 0;
		*(user_stack + 6) = state->ebx;
		*(user_stack + 7) = state->edx;
		*(user_stack + 8) = state->ecx;
		*(user_stack + 9) = state->eax;
		*(user_stack + 10) = iret->entry;

		iret->entry = task_sigjmp_crt0;
		task->task_state = TASK_STATE_RUNNING;
		return 0;
	}

	// Default handlers
	if(sig == SIGCONT && task->task_state == TASK_STATE_STOPPED) {
		task->task_state = TASK_STATE_RUNNING;
		return 0;
	}

	if(sig == SIGCHLD && task->task_state == TASK_STATE_WAITING) {
		task->task_state = TASK_STATE_RUNNING;
		return 0;
	}

	if(sig == SIGCONT || sig == SIGURG) {
		return 0;
	}

	task->task_state = TASK_STATE_TERMINATED;
	task->interrupt_yield = true;
	return 0;
}
