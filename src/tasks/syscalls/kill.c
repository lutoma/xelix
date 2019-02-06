/* kill.c: Kill Syscall
 * Copyright © 2013-2019 Lukas Martini
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

SYSCALL_HANDLER(kill) {
	task_t* task = scheduler_find(syscall.params[0]);
	if(!task) {
		sc_errno = ESRCH;
		return -1;
	}

	/* POSIX: "If sig is 0 (the null signal), error checking is performed but
	 * no signal is actually sent. The null signal can be used to check the
	 * validity of pid."
	 */
	if(!syscall.params[1]) {
		return 0;
	}

	return task_signal(task, syscall.params[1], syscall.state);
}
