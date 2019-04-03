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
#include <bitmap.h>

// From newlib
#define SIG_SETMASK 0	/* set mask with sigprocmask() */
#define SIG_BLOCK 1	/* set of signals to block */
#define SIG_UNBLOCK 2	/* set of signals to, well, unblock */

// signal.asm
extern void task_sigjmp_crt0(void);

int task_signal(task_t* task, task_t* source, int sig, isf_t* state) {
	if(sig > 32) {
		sc_errno = EINVAL;
		return -1;
	}

	if(sig == SIGKILL || sig == SIGSTOP) {
		task->task_state = (sig == SIGKILL) ? TASK_STATE_TERMINATED : TASK_STATE_STOPPED;
		task->interrupt_yield = true;
		return 0;
	}

	// Check task signal mask
	if(bit_get(task->signal_mask, sig)) {
		return 0;
	}

	struct sigaction sa = task->signal_handlers[sig];
	if((uint32_t)sa.sa_handler == SIG_IGN) {
		return 0;
	}

	if(sa.sa_handler && (uint32_t)sa.sa_handler != SIG_DFL) {
		iret_t* iret = task->kernel_stack + PAGE_SIZE - sizeof(iret_t);
		iret->user_esp -= 11 * sizeof(uint32_t);

		uint32_t* user_stack = (uint32_t*)vmem_translate(task->memory_context, iret->user_esp, false);

		// Address of signal handler and signal number as argument to it
		*user_stack = (uint32_t)sa.sa_handler;
		*(user_stack + 1) = sig;

		if(!state) {
			state = task->state;
		}

		// GP registers, will be restored by task_sigjmp_crt0 using popa
		*(user_stack + 2) = state->edi;
		*(user_stack + 3) = state->esi;
		*(user_stack + 4) = (uint32_t)state->ebp;
		*(user_stack + 5) = 0;
		*(user_stack + 6) = state->ebx;
		*(user_stack + 7) = state->edx;
		*(user_stack + 8) = state->ecx;
		*(user_stack + 9) = state->eax;

		// Current EIP, will be jumped back to after handler returns
		*(user_stack + 10) = (uint32_t)iret->entry;
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
	task->exit_code = 0x100 | sig;
	task->interrupt_yield = true;
	return 0;
}

// Syscall API
int task_signal_syscall(task_t* source, isf_t* state, int target_pid, int sig) {
	task_t* target_task = scheduler_find(target_pid);
	if(!target_task) {
		sc_errno = ESRCH;
		return -1;
	}

	if(target_task->euid != source->euid) {
		sc_errno = EPERM;
		return -1;
	}

	/* POSIX: "If sig is 0 (the null signal), error checking is performed but
	 * no signal is actually sent. The null signal can be used to check the
	 * validity of pid."
	 */
	if(!sig) {
		return 0;
	}

	return task_signal(target_task, source, sig, state);
}

int task_sigprocmask(task_t* task, int how, uint32_t* set, uint32_t* oset) {
	if(oset) {
		memcpy(oset, &task->signal_mask, sizeof(uint32_t));
	}

	if(set) {
		if(how == SIG_SETMASK) {
			task->signal_mask = *set;
		} else if(how == SIG_BLOCK) {
			task->signal_mask |= *set;
		} else if(how == SIG_UNBLOCK) {
			task->signal_mask &= ~*set;
		}
	}
	return 0;
}

int task_sigaction(task_t* task, int sig, const struct sigaction* act,
	struct sigaction* oact) {

	if(sig > 32) {
		sc_errno = EINVAL;
		return -1;
	}

	struct sigaction* tbl_entry = &task->signal_handlers[sig];
	if(oact) {
		memcpy(oact, tbl_entry, sizeof(struct sigaction));
	}

	if(act) {
		memcpy(tbl_entry, (struct sigaction*)act, sizeof(struct sigaction));
	}

	return 0;
}
