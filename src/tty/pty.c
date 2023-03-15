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

/* FIXME
 * A lot of this code is nearly identical to that in fs/pipe.c - Should be generalized
 * This should get properly integrated with tty.c code and use struct terminal*
 * Proper close handling
 */

struct pty {
	struct term term;

	uint32_t num;
	int ptm_fd;
	int pts_fd;

	// Read buffers for each stream direction.
	struct buffer* ptm_buf;
	struct buffer* pts_buf;

	struct termios termios;
	struct winsize winsize;
	int read_done;
};

static uint8_t default_c_cc[NCCS] = {
	0,
	4, // VEOF
	'\n', // VEOL
	0177, // VERASE
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

static size_t ptm_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;

	if(!buffer_size(pty->ptm_buf) && ctx->fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

	while(!buffer_size(pty->ptm_buf)) {
		scheduler_yield();
	}

	return buffer_pop(pty->ptm_buf, dest, size);
}

static size_t pts_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;

	if(!buffer_size(pty->pts_buf) && ctx->fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

/*	vfs_file_t* write_fp = vfs_get_from_id(pty->fd[1], ctx->task);
	if(!pty->data_size && !write_fp) {
		sc_errno = EBADF;
		return -1;
	}
*/
	while(!buffer_size(pty->pts_buf)) {
		scheduler_yield();
	}

	if(pty->termios.c_lflag & ICANON) {
		if(!pty->read_done && ctx->fp->flags & O_NONBLOCK) {
			sc_errno = EAGAIN;
			return -1;
		}

		while(!pty->read_done) {
			scheduler_yield();
		}
		pty->read_done = 0;
	}

	return buffer_pop(pty->pts_buf, dest, size);
}


/* Canonical read mode input handler. Perform internal line-editing and block
 * read syscall until we encounter VEOF or VEOL.
 */
static void handle_canon(struct pty* pty, char chr) {
	// EOF / ^D
	if(chr == pty->termios.c_cc[VEOF]) {
		pty->read_done = true;
		return;
	}

	// Signal task on ^C
	/*if(chr == pty->termios.c_cc[VINTR] && pty->fg_task && pty->termios.c_lflag & ISIG) {
		task_signal(pty->fg_task, NULL, SIGINT, NULL);
	}*/

	if(chr == pty->termios.c_cc[VERASE]) {
		char _unused;
		buffer_pop(pty->pts_buf, &_unused, 1);
	} else {
		buffer_write(pty->pts_buf, &chr, 1);
	}

	if(chr == pty->termios.c_cc[VEOL]) {
		pty->read_done = true;
	}
}

static inline size_t write_ptm_buf(struct pty* pty, const char* source, size_t size) {
	if(pty->termios.c_oflag & ONLCR) {
		size_t i = 0;
		for(; i < size; i++, source++) {
			if(*source == '\n') {
				if(buffer_write(pty->ptm_buf, "\r\n", 2) != 2) {
					break;
				}
			} else {
				if(!buffer_write(pty->ptm_buf, source, 1)) {
					break;
				}
			}
		}

		return i;
	} else {
		return buffer_write(pty->ptm_buf, source, size);
	}
}

static size_t ptm_write(struct vfs_callback_ctx* ctx, void* _source, size_t size) {
	char* source = (char*)_source;
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;

	// Loop back input if ECHO is enabled
	if(pty->termios.c_lflag & ECHO) {
		write_ptm_buf(pty, source, size);
	}

	if(pty->termios.c_lflag & ICANON) {
		for(size_t i = 0; i < size; i++) {
			handle_canon(pty, source[i]);
		}
	} else {
		buffer_write(pty->pts_buf, source, size);
	}

	return size;
}

static size_t pts_write(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	return write_ptm_buf((struct pty*)ctx->fp->mount_instance, (char*)source, size);
}

static int poll_cb(struct vfs_callback_ctx* ctx, int events) {
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;
	struct buffer* buf = ctx->fp->num == pty->ptm_fd ? pty->ptm_buf : pty->pts_buf;

//	int r = events & POLLOUT;
	int r = 0;
	if(events & POLLIN && buffer_size(buf)) {
		r |= POLLIN;
	}

	return r;
}

int pty_ioctl(struct vfs_callback_ctx* ctx, int request, void* _arg) {
	struct pty* pty = (struct pty*)ctx->fp->mount_instance;
	size_t arg_size = 0;

	/* Because arg can be a buffer of varying width, we can't use the automatic
	 * syscall pointer mapping code. So do that manually here.
	 */
	switch(request) {
		case TIOCGPTN:
			arg_size = sizeof(int);
			break;
		case TIOCGWINSZ:
		case TIOCSWINSZ:
			arg_size = sizeof(struct winsize);
			break;
		case TCGETS:
		case TCSETS:
		case TCSETSW:
			arg_size = sizeof(struct termios);
			break;
		default:
			sc_errno = ENOSYS;
			return -1;
	}

	vm_alloc_t alloc;
	void* arg = vm_map(VM_KERNEL, &alloc, &ctx->task->vmem, _arg,
		arg_size, VM_MAP_USER_ONLY | VM_RW);

	if(!arg) {
		task_signal(ctx->task, NULL, SIGSEGV, NULL);
		sc_errno = EFAULT;
		return -1;
	}

	switch(request) {
		case TIOCGPTN:
			*(int*)arg = pty->pts_fd;
			break;
		case TCGETS:
			memcpy(arg, &pty->termios, sizeof(struct termios));
			break;
		case TCSETS:
		case TCSETSW:
			memcpy(&pty->termios, arg, sizeof(struct termios));
			// Allow only supported flags - can't chance c_cflag
			pty->termios.c_iflag &= ICRNL | BRKINT | IGNBRK | IEXTEN;
			pty->termios.c_oflag &= OPOST | ONLCR;
			pty->termios.c_lflag &= ICANON | ISIG | IEXTEN | ECHO | ECHOE | ECHOK;
			break;
		case TIOCGWINSZ:
			memcpy(arg, &pty->winsize, sizeof(struct winsize));
			break;
		case TIOCSWINSZ:
			memcpy(&pty->winsize, arg, sizeof(struct winsize));
			break;
	}

	vm_free(&alloc);
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
		.poll = poll_cb,
		.ioctl = pty_ioctl,
		.stat = pty_stat,
		.access = sysfs_access,
	};

	struct vfs_callbacks pts_cb = {
		.read = pts_read,
		.write = pts_write,
		.poll = poll_cb,
		.ioctl = pty_ioctl,
		.stat = pty_stat,
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

	struct pty* pty = zmalloc(sizeof(struct pty));
	pty->num = __sync_add_and_fetch(&max_pty, 1);
	memcpy(&pty->term.vfs_cb, &pts_cb, sizeof(struct vfs_callbacks));
	snprintf(pty->term.path, 30, "/dev/pts%d", pty->num + 1);
	snprintf(fd1->path, 30, "/dev/ptm%d", pty->num + 1);
	snprintf(fd2->path, 30, "/dev/pts%d", pty->num + 1);

	/* Linux defaults:
	#define TTYDEF_IFLAG    (BRKINT | ISTRIP | ICRNL | IMAXBEL | IXON | IXANY)
	#define TTYDEF_OFLAG    (OPOST | ONLCR | XTABS)
	#define TTYDEF_LFLAG    (ECHO | ICANON | ISIG | IEXTEN | ECHOE | ECHOKE | ECHOCTL)
	#define TTYDEF_CFLAG    (CREAD | CS7 | PARENB | HUPCL)
	#define TTYDEF_SPEED    (B9600)
	*/

	pty->termios.c_iflag = ICRNL;
	pty->termios.c_oflag = OPOST | ONLCR;
	pty->termios.c_cflag = CREAD | B38400;
	pty->termios.c_lflag = ICANON | ISIG | IEXTEN | ECHO | ECHOE | ECHOK;

	pty->ptm_fd = fd1->num;
	pty->ptm_buf = buffer_new(150);
	if(!pty->ptm_buf) {
		return NULL;
	}

	pty->pts_fd = fd2->num;
	pty->pts_buf = buffer_new(150);
	if(!pty->pts_buf) {
		return NULL;
	}

	memcpy(&pty->termios.c_cc, default_c_cc, sizeof(default_c_cc));

	fd1->mount_instance = (void*)pty;
	fd2->mount_instance = (void*)pty;

	char name[6];
	snprintf(&name[0], 6, "pts%d", pty->num + 1);
	sysfs_add_dev(&name[0], &pts_cb);
	return fd1;
}

void pty_init() {
	struct vfs_callbacks ptmx_cb = {
		.open = ptmx_open,
	};
	sysfs_add_dev("ptmx", &ptmx_cb);
}
