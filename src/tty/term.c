/* term.c: Terminal handling
 * Copyright © 2019-2023 Lukas Martini
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

#include <tty/keyboard.h>
#include <tty/pty.h>
#include <tty/console.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <fs/poll.h>
#include <mem/mem.h>
#include <mem/kmalloc.h>
#include <panic.h>
#include <errno.h>
#include <stdlib.h>
#include <buffer.h>
#include <log.h>

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

struct buffer* input_buffer = NULL;
struct term* term_console = NULL;

size_t term_write(struct term* term, const char* source, size_t size) {
	if(term->termios.c_oflag & ONLCR) {
		size_t i = 0;
		for(; i < size; i++, source++) {
			if(*source == '\n') {
				if(term->write_cb(term, "\r\n", 2) != 2) {
					break;
				}
			} else {
				if(!term->write_cb(term, source, 1)) {
					break;
				}
			}
		}

		return i;
	} else {
		return term->write_cb(term, source, size);
	}
}

/* Canonical read mode input handler. Perform internal line-editing and block
 * read syscall until we encounter VEOF or VEOL.
 */
static void handle_canon(struct term* term, char chr) {
	// EOF / ^D
	if(chr == term->termios.c_cc[VEOF]) {
		term->read_done = true;
		return;
	}

	if(chr == term->termios.c_cc[VEOL]) {
		term->read_done = true;
	}

	if(chr == term->termios.c_cc[VERASE]) {
		if(buffer_size(term->input_buf)) {
			char* str = "\e[D\e[K";
			term_write(term, str, strlen(str));
		}

		char _unused;
		buffer_pop(term->input_buf, &_unused, 1);
		return;
	}

	buffer_write(term->input_buf, &chr, 1);

	// Loop back input if ECHO is enabled
	if(term->termios.c_lflag & ECHO) {
		term_write(term, &chr, 1);
	}
}

size_t term_input(struct term* term, const void* _source, size_t size) {
	char* source = (char*)_source;

	for(size_t i = 0; i < size; i++) {
		char chr = source[i];
		if(chr == term->termios.c_cc[VINTR] && term->fg_task && term->termios.c_lflag & ISIG) {
			term_write(term, "^C\n", 3);
			task_signal(term->fg_task, NULL, SIGINT, NULL);
			continue;
		}

		if(term->termios.c_lflag & ICANON) {
			handle_canon(term, chr);
		} else {
			// Loop back input if ECHO is enabled
			if(term->termios.c_lflag & ECHO) {
				term_write(term, &chr, 1);
			}

			buffer_write(term->input_buf, &chr, 1);
		}
	}


	return size;
}

size_t term_vfs_write(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	struct term* term = (struct term*)ctx->fp->meta;
	return term_write(term, (char*)source, size);
}

size_t term_vfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	struct term* term = (struct term*)ctx->fp->meta;

	if(!buffer_size(term->input_buf) && ctx->fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

/*	vfs_file_t* write_fp = vfs_get_from_id(term->fd[1], ctx->task);
	if(!term->data_size && !write_fp) {
		sc_errno = EBADF;
		return -1;
	}
*/
	while(!buffer_size(term->input_buf)) {
		scheduler_yield();
	}

	if(term->termios.c_lflag & ICANON) {
		if(!term->read_done && ctx->fp->flags & O_NONBLOCK) {
			sc_errno = EAGAIN;
			return -1;
		}

		while(!term->read_done) {
			scheduler_yield();
		}
		term->read_done = 0;
	}

	return buffer_pop(term->input_buf, dest, size);
}

static int term_vfs_readlink(struct vfs_callback_ctx* ctx, char* buf, size_t size) {
	task_t* task = ctx->task;
	if(!task || !task->ctty) {
		sc_errno = ENOENT;
		return -1;
	}

	int len = MIN(strlen(task->ctty->path), size);
	memcpy(buf, task->ctty->path, len);
	return len;
}

int term_vfs_stat(struct vfs_callback_ctx* ctx, vfs_stat_t* dest) {
	dest->st_dev = 2;
	dest->st_ino = 3;
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

int term_vfs_ioctl(struct vfs_callback_ctx* ctx, int request, void* _arg) {
	struct term* term = (struct term*)ctx->fp->meta;
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
			// FIXME pty only
			*(int*)arg = term->pts_fd;
			break;
		case TCGETS:
			memcpy(arg, &term->termios, sizeof(struct termios));
			break;
		case TCSETS:
		case TCSETSW:
			memcpy(&term->termios, arg, sizeof(struct termios));
			// Allow only supported flags - can't chance c_cflag
			term->termios.c_iflag &= ICRNL | BRKINT | IGNBRK | IEXTEN;
			term->termios.c_oflag &= OPOST | ONLCR;
			term->termios.c_lflag &= ICANON | ISIG | IEXTEN | ECHO | ECHOE | ECHOK;
			break;
		case TIOCGWINSZ:
			memcpy(arg, &term->winsize, sizeof(struct winsize));
			break;
		case TIOCSWINSZ:
			memcpy(&term->winsize, arg, sizeof(struct winsize));
			break;
	}

	vm_free(&alloc);
	return 0;
}

int term_vfs_poll(struct vfs_callback_ctx* ctx, int events) {
	struct term* term = (struct term*)ctx->fp->meta;
	struct buffer* buf = term->input_buf;

//	int r = events & POLLOUT;
	int r = 0;
	if(events & POLLIN && buffer_size(buf)) {
		r |= POLLIN;
	}

	return r;
}

static vfs_file_t* term_vfs_open(struct vfs_callback_ctx* ctx, uint32_t flags);
struct vfs_callbacks term_cb = {
	.stat = term_vfs_stat,
	.ioctl = term_vfs_ioctl,
	.readlink = term_vfs_readlink,
	.open = term_vfs_open,
	.write = term_vfs_write,
	.read = term_vfs_read,
	.poll = term_vfs_poll,
	.access = sysfs_access,
};

struct term* term_new(char* name, term_write_cb_t* write_cb) {
	struct term* term = zmalloc(sizeof(struct term));
	snprintf(term->path, VFS_PATH_MAX, "/dev/%s", name);
	term->write_cb = write_cb;

	term->termios.c_iflag = ICRNL;
	term->termios.c_oflag = OPOST | ONLCR;
	term->termios.c_cflag = CREAD | B38400;
	term->termios.c_lflag = ICANON | ISIG | IEXTEN | ECHO | ECHOE | ECHOK;

	memcpy(&term->termios.c_cc, default_c_cc, sizeof(default_c_cc));

	term->input_buf = buffer_new(150);
	if(!term->input_buf) {
		kfree(term);
		return NULL;
	}

	sysfs_add_dev(name, &term_cb);
	return term;
}

static vfs_file_t* term_vfs_open(struct vfs_callback_ctx* ctx, uint32_t flags) {
	if(!ctx->task) {
		sc_errno = ENOENT;
		return NULL;
	}

	vfs_file_t* fp = vfs_alloc_fileno(ctx->task, 0);
	if(!fp) {
		return NULL;
	}

	fp->inode = 1;
	fp->type = FT_IFCHR;
	fp->meta = (int)term_console;
	// FIXME
	fp->mount_instance = term_console;
	memcpy(&fp->callbacks, &term_cb, sizeof(struct vfs_callbacks));
	return fp;
}

void term_init() {
	tty_keyboard_init();

	sysfs_add_dev("tty", &term_cb);
	sysfs_add_dev("stdin", &term_cb);
	sysfs_add_dev("stdout", &term_cb);
	sysfs_add_dev("stderr", &term_cb);

	pty_init();
}
