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

SYSCALL_HANDLER(sigaction) {
	int sig = syscall.params[0];
	if(sig > 32) {
		sc_errno = EINVAL;
		return -1;
	}

	struct sigaction* tbl_entry = &syscall.task->signal_handlers[sig];
	if(syscall.params[2]) {
		SYSCALL_SAFE_RESOLVE_PARAM(2);
		memcpy((struct sigaction*)syscall.params[2], tbl_entry, sizeof(struct sigaction));

	}

	if(syscall.params[1]) {
		SYSCALL_SAFE_RESOLVE_PARAM(1);
		memcpy(tbl_entry, (struct sigaction*)syscall.params[1], sizeof(struct sigaction));
	}
	return 0;
}
