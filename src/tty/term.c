/* term.c: Terminal handling
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

#include <tty/tty.h>
#include <tty/fbtext.h>
#include <tty/keyboard.h>
#include <tty/ecma48.h>
#include <tty/input.h>
#include <tty/pty.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <mem/palloc.h>
#include <panic.h>
#include <errno.h>
#include <stdlib.h>
#include <log.h>


static int term_stat(struct vfs_callback_ctx* ctx, vfs_stat_t* dest) {
	dest->st_dev = 2;
	dest->st_ino = 3;
	dest->st_mode = FT_IFLNK | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
	dest->st_nlink = 1;
	dest->st_blocks = 0;
	dest->st_blksize = PAGE_SIZE;
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

static int term_readlink(struct vfs_callback_ctx* ctx, char* buf, size_t size) {
	task_t* task = ctx->task;
	if(!task || !task->ctty) {
		sc_errno = ENOENT;
		return -1;
	}

	int len = MIN(strlen(task->ctty->path), size);
	memcpy(buf, task->ctty->path, len);
	return len;
}

static size_t term_write(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	serial_printf(strndup(source, size));
	return size;
}


//static vfs_file_t* term_open(struct vfs_callback_ctx* ctx, uint32_t flags);
struct vfs_callbacks term_cb = {
	.stat = term_stat,
	.readlink = term_readlink,
	//.open = term_open,
	.write = term_write,
	.access = sysfs_access,
};

#if 0
static vfs_file_t* term_open(struct vfs_callback_ctx* ctx, uint32_t flags) {
	serial_printf("term_open %s\n", ctx->orig_path);
	if(!ctx->task || !ctx->task->ctty) {
		serial_printf("open failed.\n");
		sc_errno = ENOENT;
		return NULL;
	}

	return ctx->task->ctty->vfs_cb.open(ctx, flags);
}
#endif

void term_init() {
	tty_keyboard_init();
	tty_mouse_init();

	struct tty_driver* fbtext_drv = tty_fbtext_init();
	if(!fbtext_drv) {
		panic("tty: Could not initialize fbtext driver");
	}

	tty_init();

	sysfs_add_dev("tty", &term_cb);
	sysfs_add_dev("stdin", &term_cb);
	sysfs_add_dev("stdout", &term_cb);
	sysfs_add_dev("stderr", &term_cb);

	pty_init();
	tty_gfxbus_init();
}
