/* gfxbus.c: Bus for userland gfx compositor
 * Copyright © 2020 Lukas Martini
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

/* This should all be converted to using standard mmap shared memory and fifos
 * once those are implemented
 */

#include <mem/mem.h>
#include <fs/sysfs.h>
#include <fs/poll.h>
#include <errno.h>
#include <buffer.h>
#include <string.h>
#include <log.h>

static struct buffer* buf = NULL;
static task_t* master_task = NULL;
static int last_wid = -1;

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(!buffer_size(buf) && ctx->fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

	while(!buffer_size(buf)) {
		scheduler_yield();
	}

	return buffer_pop(buf, dest, size);
}



static size_t sfs_write(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	int wr = buffer_write(buf, source, size);
	int_enable();
	while(buffer_size(buf));
	int_disable();
	return wr;
}

static int sfs_poll(struct vfs_callback_ctx* ctx, int events) {
	int_enable();
	if(events & POLLIN && buffer_size(buf)) {
		return POLLIN;
	}
	int_disable();
	return 0;
}

static int sfs_ioctl(struct vfs_callback_ctx* ctx, int request, void* _arg) {
	if(request == 0x2f01) {
		master_task = ctx->task;
		return 0;
	} else if(request == 0x2f02) {
		size_t size = (size_t)_arg;

		if(!master_task) {
			return 0;
		}

		task_t* task = ctx->task;
		int flags[] = {VM_USER | VM_RW | VM_ZERO, VM_USER | VM_RW};
		struct vm_ctx* ctx[] = {&master_task->vmem, &task->vmem};

		void* addr = vm_alloc_many(2, ctx, NULL, RDIV(size, PAGE_SIZE), NULL, flags);
		if(!addr) {
			return -1;
		}

		return (uintptr_t)addr;
	} else if(request == 0x2f03) {
		return __sync_add_and_fetch(&last_wid, 1);
	}

	sc_errno = EINVAL;
	return -1;
}


void tty_gfxbus_init() {
	struct vfs_callbacks sfs_cb = {
		.ioctl = sfs_ioctl,
		.read = sfs_read,
		.write = sfs_write,
		.poll = sfs_poll
	};

	buf = buffer_new(1500);
	if(!buf) {
		return;
	}

	sysfs_add_dev("gfxbus", &sfs_cb);
}
