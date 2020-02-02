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
#include <mem/palloc.h>
#include <buffer.h>
#include <errno.h>
#include <spinlock.h>
#include <log.h>

/* FIXME
 * A lot of this code is nearly identical to that in fs/pipe.c - Should be generalized
 * This should get properly integrated with tty.c code and use struct terminal*
 * Proper close handling
 */

struct pty {
	uint32_t num;

	// 0 = master, 1 = slave
	struct buffer* buf[2];
	int fds[2];
	struct termios termios;
};


static uint8_t default_c_cc[NCCS] = {
	0,
	4, // VEOF
	'\n', // VEOL
	'\b', // VERASE
	3, // VINTR
	21, // VKILL
	0,  // VMIN
	28, // VQUIT
	17, // VSTART
	19, // VSTOP
	26, // VSUSP
	0,
};

uint32_t max_pty = -1;

static size_t read_cb(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;

	int bno = 1;
	if(ctx->fp->num == pty->fds[0]) {
		// Read from master to slave
		bno = 0;
	}

	if(!pty->buf[bno]->size && ctx->fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

/*	vfs_file_t* write_fp = vfs_get_from_id(pty->fd[1], ctx->task);
	if(!pty->data_size && !write_fp) {
		sc_errno = EBADF;
		return -1;
	}
*/
	while(!pty->buf[bno]->size) {
		halt();
	}

	if(bno) {
		return buffer_pop(pty->buf[bno], dest, size);
	} else {
		// FIXME should only be done for canon reads
		size_t nread = 0;
		for(; nread < size; nread++) {
			char c;
			if(buffer_pop(pty->buf[0], &c, 1) < 1) {
				break;
			}

			if(c == '\n') {
				if(nread + 2 >= size) {
					return nread;
				}

				((char*)dest)[nread++] = '\r';
			}

			((char*)dest)[nread] = c;
		}

		return nread;
	}
}

static size_t write_cb(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;

	int bno = 0;
	if(ctx->fp->num == pty->fds[0]) {
		// Write from master to slave
		bno = 1;
	}

	// Loop back input if ECHO is enabled
	if(bno == 1 && pty->termios.c_lflag & ECHO) {
		buffer_write(pty->buf[0], source, size);
	}

	buffer_write(pty->buf[bno], source, size);
	return size;
}

static int poll_cb(struct vfs_callback_ctx* ctx, int events) {
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;

	int bno = 1;
	if(ctx->fp->num == pty->fds[0]) {
		// Write from master to slave
		bno = 0;
	}

	if(!spinlock_get(&pty->buf[bno]->lock, 1000)) {
		return 0;
	}

	if(events & POLLIN && pty->buf[bno]->size) {
		spinlock_release(&pty->buf[bno]->lock);
		return POLLIN;
	}

	spinlock_release(&pty->buf[bno]->lock);
	return 0;
}

int pty_ioctl(struct vfs_callback_ctx* ctx, int request, void* arg) {
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;

	switch(request) {
		case TIOCGPTN:
			*(int*)arg = pty->fds[1];
			return 0;
		case TIOCGWINSZ:;
			struct winsize* ws = (struct winsize*)arg;
			// FIXME
			ws->ws_row = 50;
			ws->ws_col = 128;
			ws->ws_xpixel = 0;
			ws->ws_ypixel = 0;
			return 0;
		case TCGETS:
			memcpy(arg, &pty->termios, sizeof(struct termios));
			return 0;
		case TCSETS:
		case TCSETSW:;
			memcpy(&pty->termios, arg, sizeof(struct termios));
			return 0;
		default:
			sc_errno = ENOSYS;
			return -1;
	}

	return 0;
}

static int pty_stat(struct vfs_callback_ctx* ctx, vfs_stat_t* dest) {
	dest->st_dev = 2;
	dest->st_ino = 8;
	dest->st_mode = FT_IFCHR;
	dest->st_mode |= S_IRUSR | S_IRGRP | S_IROTH;
	dest->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
	dest->st_nlink = 1;
	dest->st_blocks = 0;
	dest->st_blksize = 0x1000;
	dest->st_uid = 0;
	dest->st_gid = 0;
	dest->st_rdev = 0;
	dest->st_size = 0;
	uint32_t t = time_get();
	dest->st_atime = t;
	dest->st_mtime = t;
	dest->st_ctime = t;
	return 0;
}

static struct vfs_callbacks pty_cb = {
	.read = read_cb,
	.write = write_cb,
	.poll = poll_cb,
	.ioctl = pty_ioctl,
	.stat = pty_stat,
	.access = sysfs_access,
};

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
	pty->num = __sync_add_and_fetch(&max_pty, 1);;
	pty->termios.c_lflag = ECHO | ICANON | ISIG;

	pty->buf[0] = buffer_new(150);
	pty->fds[0] = fd1->num;
	pty->buf[1] = buffer_new(150);
	pty->fds[1] = fd2->num;

	memcpy(&pty->termios.c_cc, default_c_cc, sizeof(default_c_cc));

	fd1->mount_instance = (void*)pty;
	fd2->mount_instance = (void*)pty;

	char name[6];
	snprintf(&name[0], 6, "pts%d", pty->num + 1);
	sysfs_add_dev(&name[0], &pty_cb);
	return fd1;
}

void pty_init() {
	struct vfs_callbacks ptmx_cb = {
		.open = ptmx_open,
	};
	sysfs_add_dev("ptmx", &ptmx_cb);
}
