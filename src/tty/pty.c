/* pty.c: Pseudo terminals
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

#include <tty/pty.h>
#include <fs/vfs.h>
#include <fs/poll.h>
#include <fs/sysfs.h>
#include <mem/kmalloc.h>
#include <buffer.h>
#include <errno.h>
#include <log.h>

/* Pseudo terminal handling. This just extends the existing terminal handling
 * from term.c with an additional file descriptor that forms the 'remote' end
 * of the terminal (that would usually be handled by console.c for kernel TTYs)
 *
 * It also adds /dev/ptmx as interface to allocate pseudo terminals.
 */

uint32_t max_pty = -1;

static size_t ptm_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	struct term* pty = (struct term*)ctx->fp->meta;

	if(!buffer_size(pty->ptm_buf) && ctx->fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

	while(!buffer_size(pty->ptm_buf)) {
		scheduler_yield();
	}

	return buffer_pop(pty->ptm_buf, dest, size);
}

static size_t ptm_write(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	struct term* pty = (struct term*)ctx->fp->meta;
	return term_input(pty, source, size);
}

static int ptm_poll(struct vfs_callback_ctx* ctx, int events) {
	struct term* pty = (struct term*)ctx->fp->meta;
//	int r = events & POLLOUT;
	int r = 0;
	if(events & POLLIN && buffer_size(pty->ptm_buf)) {
		r |= POLLIN;
	}

	return r;
}

static size_t term_write_cb(struct term* term, const void* source, size_t size) {
	return buffer_write(term->ptm_buf, source, size);
}

static vfs_file_t* ptmx_open(struct vfs_callback_ctx* ctx, uint32_t flags) {
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

	struct vfs_callbacks ptm_cb = {
		.read = ptm_read,
		.write = ptm_write,
		.poll = ptm_poll,
		.ioctl = term_vfs_ioctl,
		.stat = term_vfs_stat,
		.access = sysfs_access,
	};

	struct vfs_callbacks pts_cb = {
		.read = term_vfs_read,
		.write = term_vfs_write,
		.poll = term_vfs_poll,
		.ioctl = term_vfs_ioctl,
		.stat = term_vfs_stat,
		.access = sysfs_access,
	};

	memcpy(&fd1->callbacks, &ptm_cb, sizeof(struct vfs_callbacks));
	memcpy(&fd2->callbacks, &pts_cb, sizeof(struct vfs_callbacks));

	fd1->inode = 1;
	fd2->inode = 2;
	fd1->type = FT_IFCHR;
	fd2->type = FT_IFCHR;

	// Would normally be set in vfs_open, but fd2 doesn't go through there
	fd2->mp = ctx->mp;
	fd2->flags = flags;

	int pty_num = __sync_add_and_fetch(&max_pty, 1);

	char name[6];
	snprintf(&name[0], 6, "pts%d", pty_num + 1);

	struct term* pty = term_new(&name[0], term_write_cb);
	pty->num = pty_num;
	snprintf(fd1->path, 30, "/dev/ptm%d", pty->num + 1);
	snprintf(fd2->path, 30, "/dev/pts%d", pty->num + 1);

	pty->ptm_buf = buffer_new(150);
	if(!pty->ptm_buf) {
		kfree(pty);
		return NULL;
	}

	pty->ptm_fd = fd1->num;
	pty->pts_fd = fd2->num;

	fd1->meta = (void*)pty;
	fd2->meta = (void*)pty;
	return fd1;
}

void pty_init() {
	struct vfs_callbacks ptmx_cb = {
		.open = ptmx_open,
	};
	sysfs_add_dev("ptmx", &ptmx_cb);
}
