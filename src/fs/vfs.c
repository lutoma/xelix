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
#include <memory/kmalloc.h>
#include <string.h>
#include <list.h>
#include <print.h>
#include <spinlock.h>
#include <errno.h>
#include <time.h>
#include <fs/null.h>
#include <fs/sysfs.h>

#ifdef VFS_DEBUG
# define debug(fmt, args...) log(LOG_DEBUG, "vfs: %3d %-20s %-13s %5d %-25s " fmt, \
	fp->task ? fp->task->pid : 0, \
	fp->task ? fp->task->name : "kernel", \
	__func__ + 4, fp->num, fp->path, args)
#else
# define debug(args...)
#endif

struct mountpoint {
	char path[265];
	void* instance;
	char* dev;
	char* type;
	struct vfs_callbacks callbacks;
};

struct mountpoint mountpoints[VFS_MAX_MOUNTPOINTS];
vfs_file_t kernel_files[VFS_MAX_OPENFILES];
uint32_t last_mountpoint = -1;
uint32_t last_dir = 0;
static spinlock_t file_open_lock;

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
	if(id < 0 || !spinlock_get(&file_open_lock, 200)) {
		return NULL;
	}

	vfs_file_t* fd = task? &task->files[id] : &kernel_files[id];
	if(!fd->inode) {
		return NULL;
	}

	spinlock_release(&file_open_lock);
	return fd;
}

static int get_mountpoint(char* path, char** mount_path) {
	*mount_path = path;
	size_t longest_match = 0;
	int mp_num = -1;

	for(int i = 0; i <= last_mountpoint; i++) {
		struct mountpoint cur_mp = mountpoints[i];
		size_t plen = strlen(cur_mp.path);

		if(!strncmp(path, cur_mp.path, plen - 1) && plen > longest_match) {
			longest_match = plen;
			mp_num = i;

			if(strlen(path) - plen > 0) {
				*mount_path = path + plen;
			} else {
				*mount_path = "/";
			}
		}
	}

	return mp_num;
}

vfs_file_t* vfs_open(const char* orig_path, uint32_t flags, task_t* task) {
	#ifdef VFS_DEBUG
	log(LOG_DEBUG, "vfs: %3d %-20s %-13s %5d %-25s\n",
		task ? task->pid : 0,
		task ? task->name : "kernel",
		"open", 0, orig_path);
	#endif

	if(!orig_path || !strcmp(orig_path, "")) {
		log(LOG_ERR, "vfs: vfs_open called with empty path.\n");
		sc_errno = ENOENT;
		return NULL;
	}

	if((flags & O_EXCL) && !(flags & O_CREAT)) {
		sc_errno = EINVAL;
		return NULL;
	}

	if(!spinlock_get(&file_open_lock, 200)) {
		sc_errno = EAGAIN;
		return NULL;
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
		spinlock_release(&file_open_lock);
		sc_errno = ENOENT;
		return NULL;
	}

	struct mountpoint mp = mountpoints[mp_num];
	if(!mp.callbacks.open) {
		sc_errno = ENOSYS;
		return NULL;
	}

	uint32_t inode = mp.callbacks.open(mount_path, flags, mp.instance);
	if(!inode) {
		kfree(path);
		spinlock_release(&file_open_lock);
		sc_errno = ENOENT;
		return NULL;
	}

	vfs_file_t* fdir = task ? task->files : kernel_files;
	uint32_t num = 0;
	for(; num < VFS_MAX_OPENFILES; num++) {
		if(!fdir[num].inode) {
			break;
		}
	}

	if(num >= VFS_MAX_OPENFILES) {
		kfree(path);
		spinlock_release(&file_open_lock);
		sc_errno = ENFILE;
		return NULL;
	}

	vfs_file_t* fp = task ? &fdir[num] : &fdir[num];
	fp->num = num;
	strcpy(fp->path, path);
	strcpy(fp->mount_path, mount_path);
	kfree(path);
	fp->mount_instance = mp.instance;
	fp->offset = 0;
	fp->mountpoint = mp_num;
	fp->inode = inode;
	fp->task = task;
	fp->flags = flags;

	spinlock_release(&file_open_lock);
	return fp;
}

int vfs_stat(vfs_file_t* fp, vfs_stat_t* dest) {
	debug("\n", NULL);
	strncpy(vfs_last_read_attempt, fp->path, 512);
	struct mountpoint mp = mountpoints[fp->mountpoint];
	if(!mp.callbacks.stat) {
		sc_errno = ENOSYS;
		return -1;
	}

	int r = mp.callbacks.stat(fp, dest);

	/*printf("%s uid=%d, gid=%d, size=%d, ft=%s mode=%s\n", fp->mount_path, dest->st_uid,
		dest->st_gid, dest->st_size, vfs_filetype_to_verbose(vfs_mode_to_filetype(dest->st_mode)),
		vfs_get_verbose_permissions(dest->st_mode));
	*/

	return r;
}

size_t vfs_read(void* dest, size_t size, vfs_file_t* fp) {
	debug("size %d\n", size);

	if(fp->flags & O_WRONLY) {
		sc_errno = EBADF;
		return -1;
	}

	strncpy(vfs_last_read_attempt, fp->path, 512);
	struct mountpoint mp = mountpoints[fp->mountpoint];
	if(!mp.callbacks.read) {
		sc_errno = ENOSYS;
		return -1;
	}

	size_t read = mp.callbacks.read(fp, dest, size);
	fp->offset += read;
	return read;
}

size_t vfs_write(void* source, size_t size, vfs_file_t* fp) {
	debug("size %d\n", size);

	if(fp->flags & O_RDONLY) {
		sc_errno = EBADF;
		return -1;
	}

	if(!size) {
		return 0;
	}

	strncpy(vfs_last_read_attempt, fp->path, 512);
	struct mountpoint mp = mountpoints[fp->mountpoint];
	if(!mp.callbacks.write) {
		sc_errno = ENOSYS;
		return -1;
	}

	size_t written = mp.callbacks.write(fp, source, size);
	fp->offset += written;
	return written;
}

size_t vfs_getdents(vfs_file_t* fp, void* dest, size_t size) {
	debug("size %d\n", size);

	strncpy(vfs_last_read_attempt, fp->path, 512);
	struct mountpoint mp = mountpoints[fp->mountpoint];
	if(!mp.callbacks.getdents) {
		sc_errno = ENOSYS;
		return -1;
	}

	return mp.callbacks.getdents(fp, dest, size);
}


void vfs_seek(vfs_file_t* fp, size_t offset, int origin) {
	debug("offset %d origin %d\n", offset, origin);
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
			vfs_stat(fp, stat);
			fp->offset = stat->st_size + offset;
			kfree(stat);
	}
}

int vfs_close(vfs_file_t* fp) {
	// FIXME, this seems to be buggy and causes triple faults
	// Maybe related to access after the task has ended or something
	return 0;

	debug("\n", NULL);

	if(!spinlock_get(&file_open_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}

	vfs_file_t* fdir = fp->task ? fp->task->files : kernel_files;
	fdir[fp->num].inode = 0;
	spinlock_release(&file_open_lock);
	return 0;
}

int vfs_unlink(char* orig_path, task_t* task) {
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
	if(!mp.callbacks.unlink) {
		kfree(path);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = mp.callbacks.unlink(mount_path);
	kfree(path);
	return r;
}

int vfs_chmod(const char* orig_path, uint32_t mode, task_t* task) {
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
	if(!mp.callbacks.chmod) {
		kfree(path);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = mp.callbacks.chmod(mount_path, mode);
	kfree(path);
	return r;
}


int vfs_chown(const char* orig_path, uint16_t uid, uint16_t gid, task_t* task) {
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
	if(!mp.callbacks.chown) {
		kfree(path);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = mp.callbacks.chown(mount_path, uid, gid);
	kfree(path);
	return r;
}

int vfs_mkdir(const char* orig_path, uint32_t mode, task_t* task) {
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
	if(!mp.callbacks.mkdir) {
		kfree(path);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = mp.callbacks.mkdir(mount_path, mode);
	kfree(path);
	return r;
}


int vfs_access(const char* orig_path, uint32_t amode, task_t* task) {
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
		kfree(path);
		sc_errno = ENOSYS;
		return -1;
	}


	uint32_t inode = mp.callbacks.open(mount_path, O_RDONLY, mp.instance);
	if(!inode) {
		sc_errno = ENOENT;
	}

	kfree(path);
	return inode ? 0 : -1;
}


int vfs_utimes(const char* orig_path, struct timeval times[2], task_t* task) {
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
	if(!mp.callbacks.utimes) {
		kfree(path);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = mp.callbacks.utimes(mount_path, times);
	kfree(path);
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

	int r = mp.callbacks.link(mount_path, new_mount_path);
	kfree(path);
	kfree(new_path);
	return r;
}

int vfs_readlink(const char* orig_path, char* buf, size_t size, task_t* task) {
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
	if(!mp.callbacks.readlink) {
		kfree(path);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = mp.callbacks.readlink(mount_path, buf, size);
	kfree(path);
	return r;
}

int vfs_rmdir(const char* orig_path, task_t* task) {
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
	if(!mp.callbacks.rmdir) {
		kfree(path);
		sc_errno = ENOSYS;
		return -1;
	}

	int r = mp.callbacks.rmdir(mount_path);
	kfree(path);
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
	bzero(kernel_files, sizeof(kernel_files));
	vfs_null_init();
	sysfs_add_file("mounts", sfs_mounts_read, NULL, NULL);
}
