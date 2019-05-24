/* sysfs.c: System FS. Used for /dev and /sys.
 * Copyright © 2018-2019 Lukas Martini
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

#include <log.h>
#include <string.h>
#include <errno.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <tasks/task.h>
#include <time.h>

vfs_file_t* sysfs_open(char* path, uint32_t flags, void* mount_instance, task_t* task);
int sysfs_stat(char* path, vfs_stat_t* dest, void* mount_instance, task_t* task);
int sysfs_access(char* path, uint32_t amode, void* mount_instance, struct task* task);

static struct vfs_callbacks callbacks = {
	.open = sysfs_open,
	.stat = sysfs_stat,
	.access = sysfs_access,
};

static struct sysfs_file* sys_files;
static struct sysfs_file* dev_files;

static struct sysfs_file* get_file(char* path, struct sysfs_file* first) {
	if(!first) {
		return NULL;
	}

	if(*path == '/') {
		path++;
	}

	struct sysfs_file* file = first;
	while(file) {
		if(!strcmp(file->name, path)) {
			return file;
		}

		file = file->next;
	}

	return NULL;
}

int sysfs_stat(char* path, vfs_stat_t* dest, void* mount_instance, task_t* task) {
	bool is_root = !strncmp(path, "/", 2);
	struct sysfs_file* file = get_file(path, *(struct sysfs_file**)mount_instance);
	if(!is_root && !file) {
		sc_errno = ENOENT;
		return -1;
	}

	dest->st_dev = 2;
	dest->st_ino = 1;
	if(is_root) {
		dest->st_mode = FT_IFDIR | S_IXUSR | S_IRUSR | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH;
	} else {
		dest->st_mode = file ? file->type : FT_IFDIR;

		if(file->cb.read)
			dest->st_mode |= S_IRUSR | S_IRGRP | S_IROTH;
		if(file->cb.write)
			dest->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
	}
	dest->st_nlink = 1;
	dest->st_blocks = 2;
	dest->st_blksize = 1024;
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

int sysfs_access(char* path, uint32_t amode, void* mount_instance, struct task* task) {
	if(!strncmp(path, "/", 2)) {
		return 0;
	}

	// Only root dir has exec perm
	if(amode & X_OK) {
		sc_errno = EACCES;
		return -1;
	}

	if(!get_file(path, *(struct sysfs_file**)mount_instance)) {
		sc_errno = ENOENT;
		return -1;
	}
	return 0;
}

size_t sysfs_getdents(vfs_file_t* fp, void* dest, size_t size, task_t* task) {
	struct sysfs_file* file = *(struct sysfs_file**)fp->mount_instance;

	if(!file || fp->offset) {
		return 0;
	}

	vfs_dirent_t* dir = (vfs_dirent_t*)dest;
	size_t total_length = 0;

	for(int i = 2; file; i++) {
		uint32_t name_len = strlen(file->name);
		uint32_t rec_len = sizeof(vfs_dirent_t) + name_len + 1;
		if(total_length + rec_len > size) {
			break;
		}

		memcpy(dir->d_name, file->name, name_len);
		dir->d_name[name_len] = 0;
		dir->d_ino = i;
		dir->d_reclen = rec_len;

		total_length += rec_len;
		fp->offset++;
		dir = (vfs_dirent_t*)((intptr_t)dir + dir->d_reclen);
		file = file->next;
	}

	return total_length;
}

vfs_file_t* sysfs_open(char* path, uint32_t flags, void* mount_instance, task_t* task) {
	struct sysfs_file* file = get_file(path, *(struct sysfs_file**)mount_instance);
	bool is_root = !strncmp(path, "/", 2);
	if(!(file || is_root)) {
		sc_errno = ENOENT;
		return NULL;
	}

	vfs_file_t* fp = vfs_alloc_fileno(task, 0);
	fp->inode = 1;

	if(is_root) {
		fp->type = FT_IFDIR;
		memcpy(&fp->callbacks, &callbacks, sizeof(struct vfs_callbacks));
		fp->callbacks.getdents = sysfs_getdents;
	} else {
		fp->type = file->type;
		fp->meta = (uint32_t)file->meta;
		memcpy(&fp->callbacks, &file->cb, sizeof(struct vfs_callbacks));
		if(!fp->callbacks.stat) {
			fp->callbacks.stat = sysfs_stat;
		}
		if(!fp->callbacks.access) {
			fp->callbacks.access = sysfs_access;
		}
	}
	return fp;
}

static struct sysfs_file* add_file(struct sysfs_file** table, char* name,
	struct vfs_callbacks* cb) {

	struct sysfs_file* fp = zmalloc(sizeof(struct sysfs_file));
	strcpy(fp->name, name);
	fp->type = table == &sys_files ? FT_IFREG : FT_IFCHR;
	memcpy(&fp->cb, cb, sizeof(struct vfs_callbacks));
	if(*table) {
		fp->next = *table;
		(*table)->prev = fp;
	}
	*table = fp;
	return fp;
}

static void remove_file(struct sysfs_file** table, char* name) {
	struct sysfs_file* fp = get_file(name, *table);
	if(!fp) {
		return;
	}

	if(fp->prev) {
		fp->prev->next = fp->next;
	}

	if(fp->next) {
		fp->next->prev = fp->prev;
	}

	if(*table == fp) {
		*table = fp->next;
	}

	kfree(fp);
}

struct sysfs_file* sysfs_add_file(char* name, struct vfs_callbacks* cb) {
	return add_file(&sys_files, name, cb);
}

struct sysfs_file* sysfs_add_dev(char* name, struct vfs_callbacks* cb) {
	return add_file(&dev_files, name, cb);
}

void sysfs_rm_file(char* name) {
	remove_file(&sys_files, name);
}

void sysfs_rm_dev(char* name) {
	remove_file(&dev_files, name);
}

void sysfs_init() {
	vfs_mount("/sys", &sys_files, "sys", "sysfs", &callbacks);
	vfs_mount("/dev", &dev_files, "dev", "sysfs", &callbacks);
}
