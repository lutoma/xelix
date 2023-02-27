/* poll.c: VFS polling
 * Copyright © 2019-2020 Lukas Martini
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fs/poll.h>
#include <fs/vfs.h>
#include <tasks/task.h>
#include <mem/kmalloc.h>
#include <errno.h>

int vfs_poll(task_t* task, struct pollfd* fds, uint32_t nfds, int timeout) {
	int ret = 0;
	uint32_t timeout_end = 0;
	if(timeout >= 0) {
		timeout_end = timer_get_tick() + (timeout * 1000 / timer_get_rate());
	}

	// Build contexts ahead of time to avoid constantly reallocating in the loop
	struct vfs_callback_ctx** contexts = kmalloc(sizeof(void*) * nfds);
	for(int i = 0; i < nfds; i++) {
		contexts[i] = vfs_context_from_fd(fds[i].fd, task);

		if(!contexts[i] || !contexts[i]->fp) {
			sc_errno = EBADF;
			return -1;
		}

		if(!contexts[i]->fp->callbacks.poll) {
			sc_errno = ENOSYS;
			return -1;
		}
	}

	while(1) {
		for(uint32_t i = 0; i < nfds; i++) {
			int_disable();
			int r = contexts[i]->fp->callbacks.poll(contexts[i], fds[i].events);
			if(r > 0) {
				fds[i].revents = r;
				ret = 1;
				goto bye;
			}
			int_enable();
		}

		if(timeout_end && timer_get_tick() > timeout_end) {
			break;
		}

		scheduler_yield();
	}

bye:
	int_disable();
	for(int i = 0; i < nfds; i++) {
		vfs_free_context(contexts[i]);
	}
	kfree(contexts);
	return ret;
}
