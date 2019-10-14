/* pty.c: Pseudo terminals
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tty/pty.h>
#include <tty/ioctl.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <mem/kmalloc.h>
#include <errno.h>
#include <log.h>

/* FIXME
 * A lot of thise code is nearly identical to that in fs/pipe.c - Should be generalized
 * This should get properly integrated with tty.c code and use struct terminal*
 * Buffer should dynamically grow
 * Proper close handling
 */

#define PTY_BUFFER_SIZE 0x5000

struct pty {
	// 0 = master, 1 = slave
	void* buffer[2];
	uint32_t data_size[2];
	int fds[2];
};


static size_t read_cb(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;

	int bno = 1;
	if(ctx->fp->num == pty->fds[0]) {
		// Read from master to slave
		bno = 0;
	}

	if(!pty->data_size[bno] && ctx->fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

/*	vfs_file_t* write_fp = vfs_get_from_id(pty->fd[1], ctx->task);
	if(!pty->data_size && !write_fp) {
		sc_errno = EBADF;
		return -1;
	}
*/
	while(!pty->data_size[bno]) {
		halt();
	}

	if(size > pty->data_size[bno]) {
		size = pty->data_size[bno];
	}

	memcpy(dest, pty->buffer[bno], size);
	pty->data_size[bno] -= MIN(pty->data_size[bno], size);
	if(pty->data_size[bno]) {
		memmove(pty->buffer[bno], pty->buffer[bno] + size, pty->data_size[bno]);
	}

	return size;
}

static size_t write_cb(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;

	int bno = 0;
	if(ctx->fp->num == pty->fds[0]) {
		// Write from master to slave
		bno = 1;
	}

	if(pty->data_size[bno] + size > PTY_BUFFER_SIZE) {
		sc_errno = EFBIG;
		return -1;
	}

	memcpy(pty->buffer[bno] + pty->data_size[bno], source, size);
	pty->data_size[bno] += size;
	return size;
}

static int poll_cb(struct vfs_callback_ctx* ctx, int events) {
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;

	int bno = 1;
	if(ctx->fp->num == pty->fds[0]) {
		// Write from master to slave
		bno = 0;
	}

	if(events & POLLIN && pty->data_size[bno]) {
		return POLLIN;
	}
	return 0;
}

int pty_ioctl(struct vfs_callback_ctx* ctx, int request, void* arg) {
	if(request != TIOCGPTN) {
		sc_errno = ENOSYS;
		return -1;
	}

	struct pty* pty = (struct pty*)ctx->fp->mount_instance;
	*(int*)arg = pty->fds[1];
	return 0;
}

static vfs_file_t* pty_open(struct vfs_callback_ctx* ctx, uint32_t flags);
struct vfs_callbacks pty_cb = {
	.open = pty_open,
	.read = read_cb,
	.write = write_cb,
	.poll = poll_cb,
	.ioctl = pty_ioctl,
};

static vfs_file_t* pty_open(struct vfs_callback_ctx* ctx, uint32_t flags) {
	vfs_file_t* fd1 = vfs_alloc_fileno(ctx->task, 3);
	if(!fd1) {
		sc_errno = EMFILE;
		return NULL;
	}

	vfs_file_t* fd2 = vfs_alloc_fileno(ctx->task, fd1->num);
	if(!fd2) {
		vfs_close(ctx->task, fd1->num);
		sc_errno = EMFILE;
		return NULL;
	}

	memcpy(&fd1->callbacks, &pty_cb, sizeof(struct vfs_callbacks));
	memcpy(&fd2->callbacks, &pty_cb, sizeof(struct vfs_callbacks));

	fd1->inode = 1;
	fd2->inode = 2;
	fd1->type = FT_IFCHR;
	fd2->type = FT_IFCHR;

	// Would normally be set in vfs_open, but fd2 doesn't go through there
	fd2->mp = ctx->mp;
	fd2->flags = flags;

	struct pty* pty = zmalloc(sizeof(struct pty));
	pty->buffer[0] = zmalloc(PTY_BUFFER_SIZE);
	pty->buffer[1] = zmalloc(PTY_BUFFER_SIZE);

	pty->fds[0] = fd1->num;
	pty->fds[1] = fd2->num;

	fd1->mount_instance = (void*)pty;
	fd2->mount_instance = (void*)pty;
	return fd1;
}

void pty_init() {
	sysfs_add_dev("ptmx", &pty_cb);
}
