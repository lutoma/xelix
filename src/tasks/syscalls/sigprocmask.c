/* sigaction.c: sigaction Syscall
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

#include <tasks/syscall.h>
#include <errno.h>
#include <tasks/scheduler.h>
#include <tasks/signal.h>

// From newlib
#define SIG_SETMASK 0	/* set mask with sigprocmask() */
#define SIG_BLOCK 1	/* set of signals to block */
#define SIG_UNBLOCK 2	/* set of signals to, well, unblock */

SYSCALL_HANDLER(sigprocmask) {
	int how = syscall.params[0];

	if(syscall.params[2]) {
		SYSCALL_SAFE_RESOLVE_PARAM(2);
		memcpy((struct sigaction*)syscall.params[2], syscall.task->signal_mask, sizeof(uint32_t));
	}

	if(syscall.params[1]) {
		SYSCALL_SAFE_RESOLVE_PARAM(1);
		uint32_t mask = *(uint32_t*)syscall.params[1];

		if(how == SIG_SETMASK) {
			syscall.task->signal_mask = mask;
		} else if(how == SIG_BLOCK) {
			syscall.task->signal_mask |= mask;
		} else if(how == SIG_UNBLOCK) {
			syscall.task->signal_mask &= ~mask;
		}
	}
	return 0;
}
