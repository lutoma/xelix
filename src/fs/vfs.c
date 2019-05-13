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

#define VFS_GET_CB_OR_ERROR(name)											\
	char* mount_path = NULL; 												\
	struct mountpoint* mp = resolve_path(task, orig_path, &mount_path);		\
	if(!mp || !mount_path) {												\
		/* sc_errno is already set in resolve_path */						\
		return -1;															\
	}																		\
	if(!mp->callbacks. name) {												\
		kfree(mount_path);													\
		sc_errno = ENOSYS;													\
		return -1;															\
	}

struct mountpoint {
	char path[265];
	void* instance;
	char* dev;
	char* type;
	struct vfs_callbacks callbacks;
};

struct mountpoint mountpoints[VFS_MAX_MOUNTPOINTS];
vfs_file_t kernel_files[VFS_MAX_OPENFILES];
static spinlock_t kernel_file_open_lock;
uint32_t last_mountpoint = -1;
uint32_t last_dir = 0;

char* vfs_normalize_path(const char* orig_path, char* cwd) {
	if(!strcmp(orig_path, ".")) {
		return cwd;
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
	char* new_path = kmalloc(plen + 1);
	bzero(new_path, plen + 1);
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

vfs_file_t* vfs_get_from_id(int id, task_t* task) {
	vfs_file_t* fd = task? &task->files[id] : &kernel_files[id];
	if(!fd->inode) {
		return NULL;
	}
	return fd;
}

static int get_mountpoint(char* path, char** mount_path) {
	size_t longest_match = 0;
	int mp_num = -1;

	for(int i = 0; i <= last_mountpoint; i++) {
		struct mountpoint cur_mp = mountpoints[i];
		size_t plen = strlen(cur_mp.path);

		if(!strncmp(path, cur_mp.path, plen - 1) && plen > longest_match) {
			longest_match = plen;
			mp_num = i;

			if(strlen(path) - plen > 0) {
				*mount_path = strdup(path + plen);
			} else {
				*mount_path = strdup("/");
			}
		}
	}

	return mp_num;
}

static struct mountpoint* resolve_path(task_t* task, const char* orig_path, char** mount_path) {
	char* pwd = "/";
	if(task) {
		pwd = strndup(task->cwd, 265);
	}

	char* path = vfs_normalize_path(orig_path, pwd);
	int mp_num = get_mountpoint(path, mount_path);
	kfree(path);

	if(mp_num < 0) {
		sc_errno = ENOENT;
		return NULL;
	}

	return &mountpoints[mp_num];
}

vfs_file_t* vfs_alloc_fileno(task_t* task) {
	vfs_file_t* fdir = task ? task->files : kernel_files;
	spinlock_t* lock = task ? &task->file_open_lock : &kernel_file_open_lock;
	uint32_t num = 0;

	if(!spinlock_get(lock, 200)) {
		sc_errno = EAGAIN;
		return NULL;
	}

	for(; num < VFS_MAX_OPENFILES; num++) {
		if(!fdir[num].inode) {
			break;
		}
	}

	if(num >= VFS_MAX_OPENFILES) {
		spinlock_release(lock);
		sc_errno = ENFILE;
		return NULL;
	}

	/* Set to a dummy inode so two subsequent calls of this function don't
	 * return the same file.
	 */
	fdir[num].inode = 1;
	spinlock_release(lock);

	fdir[num].num = num;
	fdir[num].offset = 0;
	return &fdir[num];
}

int vfs_open(const char* orig_path, uint32_t flags, task_t* task) {
	#ifdef VFS_DEBUG
	log(LOG_DEBUG, "vfs: %3d %-20s %-13s %5d %-25s\n",
		task ? task->pid : 0,
		task ? task->name : "kernel",
		"open", 0, orig_path);
	#endif

	if(!orig_path || !strcmp(orig_path, "")) {
		log(LOG_ERR, "vfs: vfs_open called with empty path.\n");
		sc_errno = ENOENT;
		return -1;
	}

	if((flags & O_EXCL) && !(flags & O_CREAT)) {
		sc_errno = EINVAL;
		return -1;
	}

	char* pwd = "/";
	if(task) {
		pwd = strndup(task->cwd, 265);
	}

	char* path = vfs_normalize_path(orig_path, pwd);
	char* mount_path = NULL;
	int mp_num = get_mountpoint(path, &mount_path);
	if(mp_num < 0) {
		kfree(path);
		sc_errno = ENOENT;
		return -1;
	}

	struct mountpoint mp = mountpoints[mp_num];
	if(!mp.callbacks.open) {
		sc_errno = ENOSYS;
		return -1;
	}

	vfs_file_t* fp = mp.callbacks.open(mount_path, flags, mp.instance, task);
	if(!fp) {
		kfree(path);
		return -1;
	}

	strcpy(fp->path, path);
	strcpy(fp->mount_path, mount_path);
	kfree(path);
	memcpy(&fp->callbacks, &mp.callbacks, sizeof(struct vfs_callbacks));
	fp->mount_instance = mp.instance;
	fp->task = task;
	fp->flags = flags;

	if(flags & O_APPEND) {
		vfs_seek(fp->num, 0, VFS_SEEK_END, task);
	}

	return fp->num;
}

size_t vfs_read(int fd, void* dest, size_t size, task_t* task) {
	debug("size %d\n", size);

	vfs_file_t* fp = vfs_get_from_id(fd, task);
	if(!fp || fp->flags & O_WRONLY) {
		sc_errno = EBADF;
		return -1;
	}

	if(!fp->callbacks.read) {
		sc_errno = ENOSYS;
		return -1;
	}

	size_t read = fp->callbacks.read(fp, dest, size, task);
	fp->offset += read;
	return read;
}

size_t vfs_write(int fd, void* source, size_t size, task_t* task) {
	debug("size %d\n", size);

	vfs_file_t* fp = vfs_get_from_id(fd, task);
	if(!fp || fp->flags & O_RDONLY) {
		sc_errno = EBADF;
		return -1;
	}

	if(!size) {
		return 0;
	}

	if(!fp->callbacks.write) {
		sc_errno = ENOSYS;
		return -1;
	}

	size_t written = fp->callbacks.write(fp, source, size, task);
	fp->offset += written;
	return written;
}

size_t vfs_getdents(int fd, void* dest, size_t size, task_t* task) {
	debug("size %d\n", size);

	vfs_file_t* fp = vfs_get_from_id(fd, task);
	if(!fp) {
		sc_errno = EBADF;
		return -1;
	}

	if(!fp->callbacks.getdents) {
		sc_errno = ENOSYS;
		return -1;
	}

	return fp->callbacks.getdents(fp, dest, size, task);
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
		vfs_file_t* nfile = &task->files[arg3];
		memcpy(nfile, fp, sizeof(vfs_file_t));
		nfile->num = arg3;
		return 0;
	} else if(cmd == F_GETFL) {
		return fp->flags;
	} else if(cmd == F_SETFL) {
		if(arg3 & O_NONBLOCK) {
			fp->flags |= O_NONBLOCK;
		} else {
			fp->flags &= ~O_NONBLOCK;
		}
		return 0;
	}

	sc_errno = ENOSYS;
	return -1;
}

int vfs_ioctl(int fd, int request, void* arg, task_t* task) {
	debug("\n", NULL);

	vfs_file_t* fp = vfs_get_from_id(fd, task);
	if(!fp) {
		sc_errno = EBADF;
		return -1;
	}

	if(!fp->callbacks.ioctl) {
		sc_errno = ENOSYS;
		return -1;
	}

	return fp->callbacks.ioctl(fp->mount_path, request, arg, task);
}

// Legacy
int vfs_fstat(int fd, vfs_stat_t* dest, task_t* task) {
	vfs_file_t* fp = vfs_get_from_id(fd, task);
	if(!fp) {
		sc_errno = EBADF;
		return -1;
	}

	if(!fp->callbacks.stat) {
		sc_errno = ENOSYS;
		return -1;
	}
	return fp->callbacks.stat(fp->mount_path, dest, fp->mount_instance, task);
}

int vfs_stat(char* orig_path, vfs_stat_t* dest, task_t* task) {
	VFS_GET_CB_OR_ERROR(stat);
	int r = mp->callbacks.stat(mount_path, dest, mp->instance, task);
	kfree(mount_path);
	return r;
}

int vfs_close(int fd, task_t* task) {
	debug("\n", NULL);

	vfs_file_t* fp = vfs_get_from_id(fd, task);
	if(!fp) {
		sc_errno = EBADF;
		return -1;
	}

	#ifdef ENABLE_PICOTCP
	if(fp->type == FT_IFSOCK) {
		int r = net_vfs_close_cb(fp);
		if(r < 0) {
			return r;
		}
	}
	#endif

	if(fp->num > 2) {
		bzero(fp, sizeof(vfs_file_t));
	}
	return 0;
}

int vfs_unlink(char* orig_path, task_t* task) {
	VFS_GET_CB_OR_ERROR(unlink);
	int r = mp->callbacks.unlink(mount_path, task);
	kfree(mount_path);
	return r;
}

int vfs_chmod(const char* orig_path, uint32_t mode, task_t* task) {
	VFS_GET_CB_OR_ERROR(chmod);
	int r = mp->callbacks.chmod(mount_path, mode, task);
	kfree(mount_path);
	return r;
}


int vfs_chown(const char* orig_path, uint16_t uid, uint16_t gid, task_t* task) {
	VFS_GET_CB_OR_ERROR(chown);
	int r = mp->callbacks.chown(mount_path, uid, gid, task);
	kfree(mount_path);
	return r;
}

int vfs_mkdir(const char* orig_path, uint32_t mode, task_t* task) {
	VFS_GET_CB_OR_ERROR(mkdir);
	int r = mp->callbacks.mkdir(mount_path, mode, task);
	kfree(mount_path);
	return r;
}

int vfs_access(const char* orig_path, uint32_t amode, task_t* task) {
	VFS_GET_CB_OR_ERROR(access);
	int r = mp->callbacks.access(mount_path, amode, task);
	kfree(mount_path);
	return r;
}

int vfs_utimes(const char* orig_path, struct timeval times[2], task_t* task) {
	VFS_GET_CB_OR_ERROR(utimes);
	int r = mp->callbacks.utimes(mount_path, times, task);
	kfree(mount_path);
	return r;
}

int vfs_readlink(const char* orig_path, char* buf, size_t size, task_t* task) {
	VFS_GET_CB_OR_ERROR(readlink);
	int r = mp->callbacks.readlink(mount_path, buf, size, task);
	kfree(mount_path);
	return r;
}

int vfs_rmdir(const char* orig_path, task_t* task) {
	VFS_GET_CB_OR_ERROR(rmdir);
	int r = mp->callbacks.rmdir(mount_path, task);
	kfree(mount_path);
	return r;
}

int vfs_link(const char* orig_path, const char* orig_new_path, task_t* task) {
	char* pwd = "/";
	if(task) {
		pwd = strndup(task->cwd, 265);
	}

	char* path = vfs_normalize_path(orig_path, pwd);
	char* new_path = vfs_normalize_path(orig_new_path, pwd);

	char* mount_path = NULL;
	char* new_mount_path = NULL;
	int mp_num = get_mountpoint(path, &mount_path);
	int new_mp_num = get_mountpoint(new_path, &new_mount_path);

	if(mp_num < 0) {
		kfree(path);
		kfree(new_path);
		sc_errno = ENOENT;
		return -1;
	}

	if(mp_num != new_mp_num) {
		kfree(path);
		kfree(new_path);
		sc_errno = EXDEV;
		return -1;
	}

	struct mountpoint mp = mountpoints[mp_num];
	if(!mp.callbacks.link) {
		kfree(path);
		kfree(new_path);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = mp.callbacks.link(mount_path, new_mount_path, task);
	kfree(path);
	kfree(new_path);
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
	memcpy(&mountpoints[num].callbacks, callbacks, sizeof(struct vfs_callbacks));

	log(LOG_DEBUG, "vfs: Mounted %s (type %s) to %s\n", dev, type, path);
	return 0;
}

static size_t sfs_mounts_read(void* dest, size_t size, size_t offset, void* meta) {
	if(offset) {
		return 0;
	}

	size_t rsize = 0;
	for(int i = 0; i <= last_mountpoint; i++) {
		struct mountpoint mp = mountpoints[i];
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
	sysfs_add_file("mounts", sfs_mounts_read, NULL);
}
