/* vfs.c: Virtual file system
 * Copyright © 2011-2019 Lukas Martini
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

#include "vfs.h"
#include <log.h>
#include <mem/kmalloc.h>
#include <string.h>
#include <list.h>
#include <printf.h>
#include <spinlock.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <fs/null.h>
#include <fs/sysfs.h>
#include <fs/part.h>
#include <fs/ext2.h>
#include <fs/i386-ide.h>
#include <net/socket.h>

#ifdef VFS_DEBUG
# define debug(fmt, args...) log(LOG_DEBUG, "vfs: %3d %-20s %-13s %5d %-25s " fmt, \
	fp->task ? fp->task->pid : 0, \
	fp->task ? fp->task->name : "kernel", \
	__func__ + 4, fp->num, fp->path, args)
#else
# define debug(args...)
#endif

struct vfs_mountpoint mountpoints[VFS_MAX_MOUNTPOINTS];
vfs_file_t kernel_files[VFS_MAX_OPENFILES];
uint32_t last_mountpoint = -1;
uint32_t last_dir = 0;

char* vfs_normalize_path(const char* orig_path, char* cwd) {
	if(!strcmp(orig_path, ".")) {
		return strdup(cwd);
	}

	char* path;
	if(orig_path[0] != '/') {
		path = kmalloc(strlen(orig_path) + strlen(cwd) + 3);
		snprintf(path, strlen(orig_path) + strlen(cwd) + 3, "/%s/%s", cwd, orig_path);
	} else {
		path = strdup(orig_path);
	}

	size_t plen = strlen(path);
	char* ptr = path + plen - 1;
	char* new_path = zmalloc(plen + 1);
	char* nptr = new_path + plen;
	int skip = 0;
	int set = 0;

	for(; ptr >= path; ptr--) {
		if(likely(*ptr != '/')) {
			continue;
		}

		*ptr = 0;
		char* seg = ptr + 1;

		// !*seg to check for double slashes and slash at end of path
		if(!*seg || !strcmp(seg, ".")) {
			continue;
		}

		if(!strcmp(seg, "..")) {
			skip++;
			continue;
		}

		if(skip) {
			skip--;
			continue;
		}

		size_t slen = strlen(seg);
		nptr -= slen;
		memcpy(nptr, seg, slen);
		*--nptr = '/';
		set++;
	}

	if(!set) {
		*--nptr = '/';
	}

	kfree(path);
	memmove(new_path, nptr, strlen(nptr) + 1);
	return new_path;
}

vfs_file_t* vfs_get_from_id(int fd, task_t* task) {
	if(fd < 0 || fd >= ARRAY_SIZE(task->files)) {
		return NULL;
	}

	vfs_file_t* fp = task? &task->files[fd] : &kernel_files[fd];
	if(!fp->refs) {
		return NULL;
	}

	return fp->dup_target ? vfs_get_from_id(fp->dup_target, task) : fp;
}

static int get_mountpoint(char* path, char** mount_path) {
	size_t longest_match = 0;
	int mp_num = -1;
	char* mpath = NULL;

	for(int i = 0; i <= last_mountpoint; i++) {
		struct vfs_mountpoint cur_mp = mountpoints[i];
		size_t plen = strlen(cur_mp.path);

		if(!strncmp(path, cur_mp.path, plen - 1) && plen > longest_match) {
			longest_match = plen;
			mp_num = i;

			if(strlen(path) - plen > 0) {
				mpath = path + plen;
			} else {
				mpath = "/";
			}
		}
	}

	if(mount_path) {
		*mount_path = strdup(mpath);
	}
	return mp_num;
}

static inline void free_context(struct vfs_callback_ctx* ctx) {
	if(ctx->free_paths) {
		kfree(ctx->orig_path);
		kfree(ctx->path);
	}

	kfree(ctx);
}

static inline struct vfs_callback_ctx* context_from_fd(int fd, task_t* task) {
	struct vfs_callback_ctx* ctx = zmalloc(sizeof(struct vfs_callback_ctx));

	ctx->fp = vfs_get_from_id(fd, task);
	if(!ctx->fp) {
		kfree(ctx);
		return NULL;
	}

	ctx->free_paths = false;
	ctx->path = ctx->fp->mount_path;
	ctx->orig_path = ctx->fp->path;
	ctx->mp = ctx->fp->mp;
	ctx->task = task;
	return ctx;
}

static inline struct vfs_callback_ctx* context_from_path(const char* path, task_t* task) {
	struct vfs_callback_ctx* ctx = zmalloc(sizeof(struct vfs_callback_ctx));

	ctx->orig_path = vfs_normalize_path(path, task ? task->cwd : "/");
	if(!ctx->orig_path) {
		sc_errno = ENOENT;
		return NULL;
	}

	int mp_num = get_mountpoint(ctx->orig_path, &ctx->path);
	if(mp_num < 0 || !ctx->path) {
		kfree(ctx->orig_path);
		free_context(ctx);
		sc_errno = ENOENT;
		return NULL;
	}

	ctx->free_paths = true;
	ctx->mp = &mountpoints[mp_num];
	ctx->fp = NULL;
	ctx->task = task;
	return ctx;
}

vfs_file_t* vfs_alloc_fileno(task_t* task, int min) {
	vfs_file_t* file = &(task ? task->files : kernel_files)[min];

	for(int i = min; i < VFS_MAX_OPENFILES; i++, file++) {
		if(!file->refs) {
			if(likely(__sync_bool_compare_and_swap(&file->refs, 0, 1))) {
				file->refs = 1;
				file->num = i;
				return file;
			}
		}
	}

	sc_errno = ENFILE;
	return NULL;
}

int vfs_open(const char* orig_path, uint32_t flags, task_t* task) {
	#ifdef VFS_DEBUG
	log(LOG_DEBUG, "vfs: %3d %-20s %-13s %5d %-25s\n",
		task ? task->pid : 0,
		task ? task->name : "kernel",
		"open", 0, orig_path);
	#endif

	if(!orig_path || !(*orig_path)) {
		sc_errno = ENOENT;
		return -1;
	}

	if((flags & O_EXCL) && !(flags & O_CREAT)) {
		sc_errno = EINVAL;
		return -1;
	}

	struct vfs_callback_ctx* ctx = context_from_path(orig_path, task);
	if(!ctx) {
		return -1;
	}

	if(!ctx->mp->callbacks.open) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	vfs_file_t* fp = ctx->mp->callbacks.open(ctx, flags);
	if(!fp) {
		free_context(ctx);
		return -1;
	}

	strcpy(fp->path, ctx->orig_path);
	strcpy(fp->mount_path, ctx->path);
	fp->mount_instance = ctx->mp->instance;
	fp->mp = ctx->mp;
	fp->task = task;
	fp->flags = flags;

	if(flags & O_APPEND) {
		vfs_seek(fp->num, 0, VFS_SEEK_END, task);
	}

	free_context(ctx);
	return fp->num;
}

size_t vfs_read(int fd, void* dest, size_t size, task_t* task) {
	debug("size %d\n", size);

	struct vfs_callback_ctx* ctx = context_from_fd(fd, task);
	if(!ctx || !ctx->fp || ctx->fp->flags & O_WRONLY) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->fp->callbacks.read) {
		sc_errno = ENOSYS;
		return -1;
	}

	size_t read = ctx->fp->callbacks.read(ctx, dest, size);
	ctx->fp->offset += read;
	return read;
}

size_t vfs_write(int fd, void* source, size_t size, task_t* task) {
	debug("size %d\n", size);

	struct vfs_callback_ctx* ctx = context_from_fd(fd, task);
	if(!ctx || !ctx->fp || ctx->fp->flags & O_RDONLY) {
		sc_errno = EBADF;
		return -1;
	}

	if(!size) {
		return 0;
	}

	if(!ctx->fp->callbacks.write) {
		sc_errno = ENOSYS;
		return -1;
	}

	size_t written = ctx->fp->callbacks.write(ctx, source, size);
	ctx->fp->offset += written;
	return written;
}

size_t vfs_getdents(int fd, void* dest, size_t size, task_t* task) {
	debug("size %d\n", size);

	struct vfs_callback_ctx* ctx = context_from_fd(fd, task);
	if(!ctx || !ctx->fp) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->fp->callbacks.getdents) {
		sc_errno = ENOSYS;
		return -1;
	}

	return ctx->fp->callbacks.getdents(ctx, dest, size);
}

int vfs_seek(int fd, size_t offset, int origin, task_t* task) {
	debug("offset %d origin %d\n", offset, origin);

	vfs_file_t* fp = vfs_get_from_id(fd, task);
	if(!fp) {
		sc_errno = EBADF;
		return -1;
	}

	vfs_stat_t* stat;
	switch(origin) {
		case VFS_SEEK_SET:
			fp->offset = offset;
			break;
		case VFS_SEEK_CUR:
			if(offset < 0 && (offset * -1) > fp->offset) {
				fp->offset = 0;
			} else {
				fp->offset += offset;
			}
			break;
		case VFS_SEEK_END:
			stat = kmalloc(sizeof(vfs_stat_t));
			vfs_fstat(fp->num, stat, task);
			fp->offset = stat->st_size + offset;
			kfree(stat);
	}
	return 0;
}

int vfs_fcntl(int fd, int cmd, int arg3, task_t* task) {
	vfs_file_t* fp = vfs_get_from_id(fd, task);
	if(!fp) {
		sc_errno = EBADF;
		return -1;
	}

	if(cmd == F_DUPFD) {
		vfs_file_t* fp2 = vfs_alloc_fileno(task, MAX(3, arg3));
		__sync_add_and_fetch(&fp->refs, 1);
		fp2->dup_target = fp->num;
		return fp2->num;
	} else if(cmd == F_GETFL) {
		return fp->flags;
	} else if(cmd == F_SETFL) {
		if(arg3 & O_NONBLOCK) {
			fp->flags |= O_NONBLOCK;
		} else {
			fp->flags &= ~O_NONBLOCK;
		}
		return 0;
	} else if(cmd == F_GETFD) {
		return fp->flags & O_CLOEXEC;
	} else if(cmd == F_SETFD) {
		/* Only one fd flag defined so far, FD_CLOEXEC (1), so just merge this
		 * into the file status flags.
		 */
		if(arg3 & 1) {
			fp->flags |= O_CLOEXEC;
		} else {
			fp->flags &= ~O_CLOEXEC;
		}
		return 0;
	}

	sc_errno = ENOSYS;
	return -1;
}

int vfs_dup2(int fd1, int fd2, task_t* task) {
	vfs_file_t* fp1 = vfs_get_from_id(fd1, task);
	if(!fp1) {
		sc_errno = EBADF;
		return -1;
	}

	vfs_file_t* fp2 = task? &task->files[fd2] : &kernel_files[fd2];
	if(!__sync_bool_compare_and_swap(&fp2->refs, 0, 1)) {
		// Can't use fd as dup target that is already a duplication source
		if(fp2->refs > 1) {
			sc_errno = EIO;
			return -1;
		}

		// Attempt to close and try again
		vfs_close(fd2, task);
		if(!__sync_bool_compare_and_swap(&fp2->refs, 0, 1)) {
			sc_errno = EIO;
			return -1;
		}
	}

	fp2->num = fd2;
	__sync_add_and_fetch(&fp1->refs, 1);
	fp2->dup_target = fp1->dup_target ? fp1->dup_target : fp1->num;
	return 0;
}

int vfs_ioctl(int fd, int request, void* arg, task_t* task) {
	debug("\n", NULL);

	struct vfs_callback_ctx* ctx = context_from_fd(fd, task);
	if(!ctx || !ctx->fp) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->fp->callbacks.ioctl) {
		sc_errno = ENOSYS;
		return -1;
	}

	return ctx->fp->callbacks.ioctl(ctx, request, arg);
}

int vfs_fstat(int fd, vfs_stat_t* dest, task_t* task) {
	struct vfs_callback_ctx* ctx = context_from_fd(fd, task);
	if(!ctx) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->mp->callbacks.stat) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = ctx->fp->callbacks.stat(ctx, dest);
	free_context(ctx);
	return r;
}

int vfs_stat(char* orig_path, vfs_stat_t* dest, task_t* task) {
	struct vfs_callback_ctx* ctx = context_from_path(orig_path, task);
	if(!ctx) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->mp->callbacks.stat) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = ctx->mp->callbacks.stat(ctx, dest);
	free_context(ctx);
	return r;
}

int vfs_close(int fd, task_t* task) {
	debug("\n", NULL);

	vfs_file_t* fp = vfs_get_from_id(fd, task);
	if(!fp) {
		sc_errno = EBADF;
		return -1;
	}

	// Decrease ref counter and check if we've reached 0
	if(!__sync_sub_and_fetch(&fp->refs, 1)) {
		if(fp->dup_target) {
			// Decrease dup target ref counter
			vfs_close(fp->dup_target, task);
		}

		#ifdef ENABLE_PICOTCP
		if(fp->type == FT_IFSOCK) {
			int r = net_vfs_close_cb(fp);
			if(r < 0) {
				return r;
			}
		}
		#endif

		bzero(fp, sizeof(vfs_file_t));
	}
	return 0;
}

int vfs_unlink(char* orig_path, task_t* task) {
	struct vfs_callback_ctx* ctx = context_from_path(orig_path, task);
	if(!ctx) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->mp->callbacks.unlink) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = ctx->mp->callbacks.unlink(ctx);
	free_context(ctx);
	return r;
}

int vfs_chmod(const char* orig_path, uint32_t mode, task_t* task) {
	struct vfs_callback_ctx* ctx = context_from_path(orig_path, task);
	if(!ctx) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->mp->callbacks.chmod) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = ctx->mp->callbacks.chmod(ctx, mode);
	free_context(ctx);
	return r;
}

int vfs_chown(const char* orig_path, uint16_t uid, uint16_t gid, task_t* task) {
	struct vfs_callback_ctx* ctx = context_from_path(orig_path, task);
	if(!ctx) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->mp->callbacks.chown) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = ctx->mp->callbacks.chown(ctx, uid, gid);
	free_context(ctx);
	return r;
}

int vfs_mkdir(const char* orig_path, uint32_t mode, task_t* task) {
	struct vfs_callback_ctx* ctx = context_from_path(orig_path, task);
	if(!ctx) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->mp->callbacks.mkdir) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = ctx->mp->callbacks.mkdir(ctx, mode);
	free_context(ctx);
	return r;
}

int vfs_access(const char* orig_path, uint32_t amode, task_t* task) {
	struct vfs_callback_ctx* ctx = context_from_path(orig_path, task);
	if(!ctx) {
		return -1;
	}

	if(!ctx->mp->callbacks.access) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = ctx->mp->callbacks.access(ctx, amode);
	free_context(ctx);
	return r;
}

int vfs_utimes(const char* orig_path, struct timeval times[2], task_t* task) {
	struct vfs_callback_ctx* ctx = context_from_path(orig_path, task);
	if(!ctx) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->mp->callbacks.utimes) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = ctx->mp->callbacks.utimes(ctx, times);
	free_context(ctx);
	return r;
}

int vfs_readlink(const char* path, char* buf, size_t size, task_t* task) {
	struct vfs_callback_ctx* ctx = context_from_path(path, task);
	if(!ctx) {
		return -1;
	}

	if(!ctx->mp->callbacks.readlink) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = ctx->mp->callbacks.readlink(ctx, buf, size);
	free_context(ctx);
	return r;
}

int vfs_rmdir(const char* orig_path, task_t* task) {
	struct vfs_callback_ctx* ctx = context_from_path(orig_path, task);
	if(!ctx) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->mp->callbacks.rmdir) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = ctx->mp->callbacks.rmdir(ctx);
	free_context(ctx);
	return r;
}

int vfs_poll(struct pollfd* fds, uint32_t nfds, int timeout, task_t* task) {
	uint32_t timeout_end = 0;
	if(timeout >= 0) {
		timeout_end = timer_get_tick() + (timeout * 1000 / timer_get_rate());
	}

	while(1) {
		for(uint32_t i = 0; i < nfds; i++) {
			struct vfs_callback_ctx* ctx = context_from_fd(fds[i].fd, task);
			if(!ctx || !ctx->fp) {
				free_context(ctx);
				sc_errno = EBADF;
				return -1;
			}

			if(!ctx->fp->callbacks.ioctl) {
				free_context(ctx);
				sc_errno = ENOSYS;
				return -1;
			}

			int r = ctx->fp->callbacks.poll(ctx, fds[i].events);
			free_context(ctx);
			if(r) {
				fds[i].revents = r;
				return 1;
			}
		}

		if(timeout_end && timer_get_tick() > timeout_end) {
			break;
		}
		halt();
	}
	return 0;
}

int vfs_link(const char* orig_path, const char* orig_new_path, task_t* task) {
	struct vfs_callback_ctx* ctx = context_from_path(orig_path, task);
	if(!ctx) {
		sc_errno = EBADF;
		return -1;
	}

	if(!ctx->mp->callbacks.link) {
		free_context(ctx);
		sc_errno = ENOSYS;
		return -1;
	}

	char* new_path = vfs_normalize_path(orig_new_path, task ? task->cwd : "/");
	char* new_mount_path = NULL;
	int new_mp_num = get_mountpoint(new_path, &new_mount_path);

	if(ctx->mp->num != new_mp_num) {
		kfree(new_mount_path);
		free_context(ctx);
		sc_errno = EXDEV;
		return -1;
	}

	int r = ctx->mp->callbacks.link(ctx, new_mount_path);
	kfree(new_mount_path);
	free_context(ctx);
	return r;
}

int vfs_mount(char* path, void* instance, char* dev, char* type,
	struct vfs_callbacks* callbacks) {

	if(!path || !strncmp(path, "", 1)) {
		log(LOG_ERR, "vfs: vfs_mount called with empty path.\n");
		return -1;
	}

	if(!callbacks) {
		log(LOG_ERR, "vfs: vfs_mount missing callbacks\n");
		return -1;
	}

	uint32_t num;
	spinlock_cmd(num = ++last_mountpoint, 20, -1);

	strcpy(mountpoints[num].path, path);
	mountpoints[num].instance = instance;
	mountpoints[num].dev = dev;
	mountpoints[num].type = type;
	mountpoints[num].num = num;
	memcpy(&mountpoints[num].callbacks, callbacks, sizeof(struct vfs_callbacks));

	log(LOG_DEBUG, "vfs: Mounted %s (type %s) to %s\n", dev, type, path);
	return 0;
}

static size_t sfs_mounts_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	size_t rsize = 0;
	for(int i = 0; i <= last_mountpoint; i++) {
		struct vfs_mountpoint mp = mountpoints[i];
		sysfs_printf("%s %s %s rw,noatime 0 0\n", mp.dev, mp.path, mp.type);
	}
	return rsize;
}

void vfs_init() {
	ide_init();
	part_init();
	sysfs_init();

	#ifdef ENABLE_EXT2
	ext2_init();
	#endif

	bzero(kernel_files, sizeof(kernel_files));
	vfs_null_init();

	struct vfs_callbacks sfs_cb = {
		.read = sfs_mounts_read,
	};
	sysfs_add_file("mounts", &sfs_cb);
}
