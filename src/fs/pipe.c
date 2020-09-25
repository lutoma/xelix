/* pipe.c: Inter-process pipes
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

#include "pipe.h"
#include <fs/vfs.h>
#include <fs/poll.h>
#include <errno.h>
#include <tasks/task.h>
#include <mem/kmalloc.h>
#include <buffer.h>

struct pipe {
	struct buffer* buf;
	int fd[2];
};

static size_t pipe_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	struct pipe* pipe = (struct pipe*)ctx->fp->mount_instance;

	if(!buffer_size(pipe->buf) && ctx->fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

	vfs_file_t* write_fp = vfs_get_from_id(pipe->fd[1], ctx->task);
	if(!buffer_size(pipe->buf) && !write_fp) {
		sc_errno = EBADF;
		return -1;
	}

	while(!buffer_size(pipe->buf)) {
		halt();
	}

	return buffer_pop(pipe->buf, dest, size);
}

static size_t pipe_write(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	struct pipe* pipe = (struct pipe*)ctx->fp->mount_instance;
	return buffer_write(pipe->buf, source, size);
}

static int pipe_poll(struct vfs_callback_ctx* ctx, int events) {
	struct pipe* pipe = (struct pipe*)ctx->fp->mount_instance;
	if(!pipe) {
		sc_errno = EINVAL;
		return -1;
	}

	int_enable();
	if(events & POLLIN && buffer_size(pipe->buf)) {
		return POLLIN;
	}
	int_disable();
	return 0;
}

int vfs_pipe(task_t* task, int fildes[2]) {
	vfs_file_t* fd1 = vfs_alloc_fileno(task, 3);
	if(!fd1) {
		sc_errno = EMFILE;
		return -1;
	}

	vfs_file_t* fd2 = vfs_alloc_fileno(task, fd1->num);
	if(!fd2) {
		vfs_close(task, fd1->num);
		sc_errno = EMFILE;
		return -1;
	}

	struct pipe* pipe = zmalloc(sizeof(struct pipe));
	pipe->buf = buffer_new(150);
	pipe->fd[0] = fd1->num;
	pipe->fd[1] = fd2->num;

	fd1->callbacks.read = pipe_read;
	fd1->callbacks.poll = pipe_poll;
	fd2->callbacks.write = pipe_write;
	fd2->callbacks.poll = pipe_poll;
	fd1->flags = O_RDONLY;
	fd2->flags = O_WRONLY;
	fd1->mount_instance = (void*)pipe;
	fd2->mount_instance = (void*)pipe;
	fd1->type = FT_IFPIPE;
	fd2->type = FT_IFPIPE;

	fildes[0] = fd1->num;
	fildes[1] = fd2->num;
	return 0;
}
