/* sysfs.c: System FS. Used for /dev and /sys.
 * Copyright © 2018 Lukas Martini
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
#include <memory/kmalloc.h>
#include <fs/vfs.h>
#include <print.h>
#include "sysfs.h"

struct sysfs_file {
	char name[40];
	sysfs_read_callback_t read_cb;
	struct sysfs_file* next;
};

struct sysfs_file* sys_files;
struct sysfs_file* dev_files;

static struct sysfs_file* get_file(char* path, struct sysfs_file* first) {
	if(!first) {
		return NULL;
	}

	// Chop off leading /
	path++;

	struct sysfs_file* file = first;
	while(file) {
		if(!strcmp(file->name, path)) {
			return file;
		}

		file = file->next;
	}

	return NULL;
}

uint32_t sysfs_open(char* path, void* mount_instance) {
	if(!strncmp(path, "/", 2) || get_file(path, *(struct sysfs_file**)mount_instance)) {
		return 1;
	}
	return 0;
}

int sysfs_stat(vfs_file_t* fp, vfs_stat_t* dest) {
	bool is_root = !strncmp(fp->mount_path, "/", 2);
	if(is_root || !get_file(fp->mount_path, *(struct sysfs_file**)fp->mount_instance)) {
		return -1;
	}

	dest->st_dev = 2;
	dest->st_ino = 1;
	dest->st_mode = (is_root ? FT_IFDIR | S_IXUSR : FT_IFREG) | S_IRUSR | S_IWUSR;
	dest->st_nlink = 0;
	dest->st_uid = 0;
	dest->st_gid = 0;
	dest->st_rdev = 0;
	dest->st_size = 0;
	dest->st_atime = 0;
	dest->st_mtime = 0;
	dest->st_ctime = 0;
	return 0;
}

size_t sysfs_read_file(vfs_file_t* fp, void* dest, size_t size) {
	if(fp->offset) {
		return 0;
	}

	struct sysfs_file* file = get_file(fp->mount_path, *(struct sysfs_file**)fp->mount_instance);
	if(!file || !file->read_cb) {
		return 0;
	}

	return file->read_cb(dest, size);
}

size_t sysfs_getdents(vfs_file_t* fp, void* dest, size_t size) {
	struct sysfs_file* file = *(struct sysfs_file**)fp->mount_instance;

	if(!file) {
		return 0;
	}

	vfs_dirent_t* dir = (vfs_dirent_t*)dest;
	size_t total_length = 0;

	for(int i = 2; file; i++) {
		dir->name_len = strlen(file->name);
		memcpy(dir->name, file->name, dir->name_len + 1);
		dir->inode = i;
		dir->record_len = sizeof(vfs_dirent_t) + dir->name_len;

		total_length += dir->record_len;
		dir = (vfs_dirent_t*)((intptr_t)dir + dir->record_len);
		file = file->next;
	}

	return total_length;
}

static void add_file(struct sysfs_file** table, char* name, sysfs_read_callback_t read_cb) {
	struct sysfs_file* fp = kmalloc(sizeof(struct sysfs_file));
	strcpy(fp->name, name);
	fp->read_cb = read_cb;
	fp->next = *table ? *table : NULL;
	*table = fp;
}

void sysfs_add_file(char* name, sysfs_read_callback_t read_cb) {
	add_file(&sys_files, name, read_cb);
}

void sysfs_add_dev(char* name, sysfs_read_callback_t read_cb) {
	add_file(&dev_files, name, read_cb);
}

void sysfs_init() {
	vfs_mount("/sys", &sys_files, "sys", "sysfs", sysfs_open, sysfs_stat, sysfs_read_file, sysfs_getdents);
	vfs_mount("/dev", &dev_files, "dev", "sysfs", sysfs_open, sysfs_stat, sysfs_read_file, sysfs_getdents);
}
