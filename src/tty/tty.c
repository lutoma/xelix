/* tty.c: Terminal emulation
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

#include <tty/tty.h>
#include <tty/fbtext.h>
#include <tty/keyboard.h>
#include <tty/ecma48.h>
#include <tty/input.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <mem/kmalloc.h>
#include <panic.h>
#include <errno.h>
#include <stdlib.h>
#include <log.h>

uint8_t default_c_cc[NCCS] = {
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

static inline void handle_nonprintable(struct terminal* term, char chr) {
	if(chr == term->termios.c_cc[VEOL]) {
		term->cur_row++;
		term->cur_col = 0;

		if(term->cur_row >= term->drv->rows) {
			term->drv->scroll_line(term);
			term->cur_row--;
		}
		return;
	}

	if(chr == term->termios.c_cc[VERASE] || chr == 0x7f) {
		//term->scrollback_end--;
		// FIXME limit checking
		term->drv->write(term, --term->cur_col, term->cur_row, ' ', false);
		return;
	}

	if(chr == '\t') {
		term->cur_col += 8 - (term->cur_col % 8);
		return;
	}

	if(chr == term->termios.c_cc[VINTR]) {
		tty_write(NULL, "^C\n", 3);
	}
}

size_t tty_write(struct terminal* term, char* source, size_t size) {
	if(!term || !term->drv) {
		return 0;
	}

	bool remove_cursor = false;
	for(int i = 0; i < size; i++) {
		char chr = source[i];

		if(chr == '\e') {
			size_t skip = tty_handle_escape_seq(term, source + i, size - i);
			if(skip) {
				remove_cursor = true;
				i += skip;
				continue;
			}
		}

		if(term->cur_col >= term->drv->cols) {
			remove_cursor = true;
			handle_nonprintable(term, term->termios.c_cc[VEOL]);
			i--;
			continue;
		}

		if(chr > 31 && chr < 127) {
			term->last_char = chr;
			term->drv->write(term, term->cur_col, term->cur_row, chr, term->write_bdc);
			term->cur_col++;
		} else {
			remove_cursor = true;
			handle_nonprintable(term, chr);
		}
	}

	term->drv->set_cursor(term, term->cur_col, term->cur_row, remove_cursor);
	return size;
}

void tty_switch(int n) {
		struct terminal* new_tty = &ttys[n];
		new_tty->drv->rerender(active_tty, new_tty);
		active_tty = new_tty;
}

struct terminal* tty_from_path(const char* path, task_t* task, int* is_link) {
	if(!strcmp(path, "/console")) {
		return &ttys[0];
	}

	if(!strcmp(path, "/stdin") || !strcmp(path, "/stdout") ||
		!strcmp(path, "/stderr") || !strcmp(path, "/tty") ||
		!strcmp(path, "/tty0")) {

		if(is_link) {
			*is_link = 1;
		}
		return task->ctty;
	}

	// 4 chars prefix, path format is "/ttyN"
	if(strlen(path) < 5) {
		return NULL;
	}

	int n = atoi(path + 4) - 1;
	if(n < 0 || n > TTY_NUM) {
		return NULL;
	}

	return &ttys[n];
}

/* Sysfs callbacks */
static size_t sfs_write(struct vfs_file* fp, void* source, size_t size, struct task* rtask) {
	return tty_write(rtask ? rtask->ctty : &ttys[0], (char*)source, size);
}
static size_t sfs_read(struct vfs_file* fp, void* dest, size_t size, struct task* rtask) {
	return tty_read(rtask ? rtask->ctty : &ttys[0], (char*)dest, size);
}

static int tty_stat(char* path, vfs_stat_t* dest, void* mount_instance, struct task* task) {
	int is_link = 0;
	struct terminal* term = tty_from_path(path, task, &is_link);
	if(!term) {
		sc_errno = ENOENT;
		return -1;
	}

	dest->st_dev = 2;
	dest->st_ino = 3;
	dest->st_mode = is_link ? FT_IFLNK : FT_IFCHR;
	dest->st_mode |= S_IRUSR | S_IRGRP | S_IROTH;
	dest->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
	dest->st_nlink = 1;
	dest->st_blocks = 0;
	dest->st_blksize = 1024;
	dest->st_uid = 0;
	dest->st_gid = 0;
	dest->st_rdev = 0;
	dest->st_size = term->read_len;
	uint32_t t = time_get();
	dest->st_atime = t;
	dest->st_mtime = t;
	dest->st_ctime = t;
	return 0;
}

static int tty_readlink(const char* path, char* buf, size_t size, void* mount_instance, struct task* task) {
	int is_link = 0;
	struct terminal* term = tty_from_path(path, task, &is_link);
	if(!is_link) {
		sc_errno = EINVAL;
		return -1;
	}

	int len = MIN(strlen(term->path), size);
	memcpy(buf, term->path, len);
	return len;
}


static vfs_file_t* tty_open(char* path, uint32_t flags, void* mount_instance, struct task* task);
struct vfs_callbacks tty_cb = {
	.open = tty_open,
	.read = sfs_read,
	.write = sfs_write,
	.ioctl = tty_ioctl,
	.poll = tty_poll,
	.stat = tty_stat,
	.readlink = tty_readlink,
	.access = sysfs_access,
};

static vfs_file_t* tty_open(char* path, uint32_t flags, void* mount_instance, struct task* task) {
	int is_link = 0;
	struct terminal* term = tty_from_path(path, task, &is_link);

	vfs_file_t* fp = vfs_alloc_fileno(task, 0);
	fp->inode = 1;
	fp->type = is_link ? FT_IFLNK : FT_IFCHR;
	memcpy(&fp->callbacks, &tty_cb, sizeof(struct vfs_callbacks));

	if(!is_link && task && !(flags & O_NOCTTY)) {
		task->ctty = term;
	}
	return fp;
}

void tty_init() {
	for(int i = 0; i < 10; i++) {
		struct terminal* tty = &ttys[i];
		tty->num = i;
		tty->fg_color = FG_COLOR_DEFAULT;
		tty->bg_color = BG_COLOR_DEFAULT;

		tty->termios.c_lflag = ECHO | ICANON | ISIG;
		memcpy(tty->termios.c_cc, default_c_cc, sizeof(default_c_cc));
		snprintf(tty->path, 30, "/dev/tty%d", i + 1);

		char name[6];
		snprintf(&name[0], 6, "tty%d", i + 1);
		sysfs_add_dev(&name[0], &tty_cb);
	}

	active_tty = &ttys[0];
	tty_keyboard_init();
	struct tty_driver* fbtext_drv = tty_fbtext_init();
	if(!fbtext_drv) {
		panic("tty: Could not initialize fbtext driver");
	}

	for(int i = 0; i < 10; i++) {
		ttys[i].drv = fbtext_drv;
		ttys[i].drv_buf = kmalloc(fbtext_drv->buf_size);
		fbtext_drv->clear(&ttys[i], 0, 0, fbtext_drv->cols, fbtext_drv->rows);
	}

	log(LOG_INFO, "tty: Can render %d columns, %d rows\n", fbtext_drv->cols, fbtext_drv->rows);
	sysfs_add_dev("console", &tty_cb);
	sysfs_add_dev("tty", &tty_cb);
	sysfs_add_dev("tty0", &tty_cb);
	sysfs_add_dev("stdin", &tty_cb);
	sysfs_add_dev("stdout", &tty_cb);
	sysfs_add_dev("stderr", &tty_cb);
}
