/* vfs.c: Virtual file system
 * Copyright © 2011-2018 Lukas Martini
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
#include <lib/log.h>
#include <memory/kmalloc.h>
#include <lib/string.h>
#include <lib/list.h>
#include <lib/spinlock.h>

struct mountpoint {
	char path[265];
	void* instance;
	char* dev;
	char* type;
	vfs_open_callback_t open_callback;
	vfs_read_callback_t read_callback;
	vfs_getdents_callback_t getdents_callback;
};

struct mountpoint mountpoints[VFS_MAX_MOUNTPOINTS];
vfs_file_t files[VFS_MAX_OPENFILES];
uint32_t last_mountpoint = -1;
uint32_t last_file = -1;
uint32_t last_dir = 0;
static spinlock_t file_open_lock;


vfs_file_t* vfs_get_from_id(uint32_t id) {
	if(id < 1 || id > last_file || !spinlock_get(&file_open_lock, 30)) {
		return NULL;
	}

	vfs_file_t* fd = &files[id];
	spinlock_release(&file_open_lock);

	return fd;
}

vfs_file_t* vfs_open(char* path)
{
	if(!path || !strcmp(path, "")) {
		log(LOG_ERR, "vfs: vfs_open called with empty path.\n");
		return NULL;
	}

	if(!spinlock_get(&file_open_lock, 30)) {
		return NULL;
	}

	int mp_num = -1;
	struct mountpoint mp = mountpoints[0];
	size_t longest_match = 0;
	char* mount_path = path;

	for(int i = 0; i <= last_mountpoint; i++) {
		struct mountpoint cur_mp = mountpoints[i];
		size_t plen = strlen(cur_mp.path);

		if(!strncmp(path, cur_mp.path, plen - 1) && plen > longest_match) {
			longest_match = plen;
			mp_num = i;
			mp = cur_mp;

			if(strlen(path) - plen > 0) {
				mount_path = path + plen;
			} else {
				mount_path = "/";
			}
		}
	}

	if(mp_num < 0) {
		return NULL;
	}

	uint32_t inode = mp.open_callback(mount_path, mp.instance);
	if(!inode) {
		spinlock_release(&file_open_lock);
		return NULL;
	}

	uint32_t num = (last_file == -1 ? last_file = 3 : ++last_file);

	files[num].num = num;
	strcpy(files[num].path, path);
	strcpy(files[num].mount_path, mount_path);
	files[num].mount_instance = mp.instance;
	files[num].offset = 0;
	files[num].mountpoint = mp_num;
	files[num].inode = inode;

	spinlock_release(&file_open_lock);
	return &files[num];
}

size_t vfs_read(void* dest, size_t size, vfs_file_t* fp) {
	strncpy(vfs_last_read_attempt, fp->path, 512);
	struct mountpoint mp = mountpoints[fp->mountpoint];
	size_t read = mp.read_callback(fp, dest, size);
	fp->offset += size;

	return read;
}

size_t vfs_getdents(vfs_file_t* dir, void* dest, size_t size) {
	if(dir->offset) {
		return 0;
	}
	strncpy(vfs_last_read_attempt, dir->path, 512);
	struct mountpoint mp = mountpoints[dir->mountpoint];
	size_t read = mp.getdents_callback(dir, dest, size);
	dir->offset += read;
	return read;
}


void vfs_seek(vfs_file_t* fp, size_t offset, int origin) {
	switch(origin) {
		case VFS_SEEK_SET:
			fp->offset = offset;
			break;
		case VFS_SEEK_CUR:
			fp->offset += offset;
			break;
		case VFS_SEEK_END:
			log(LOG_WARN, "vfs_seek with an origin of VFS_SEEK_END is not supported so far!\n");
	}
}

int vfs_mount(char* path, void* instance, char* dev, char* type,
	vfs_open_callback_t open_callback, vfs_read_callback_t read_callback,
	vfs_getdents_callback_t getdents_callback) {

	if(!path || !strncmp(path, "", 1)) {
		log(LOG_ERR, "vfs: vfs_mount called with empty path.\n");
		return -1;
	}

	if(!open_callback || !read_callback || !getdents_callback) {
		log(LOG_ERR, "vfs: vfs_mount missing callback\n");
		return -1;
	}

	uint32_t num;
	spinlock_cmd(num = ++last_mountpoint, 20, -1);

	strcpy(mountpoints[num].path, path);
	mountpoints[num].instance = instance;
	mountpoints[num].dev = dev;
	mountpoints[num].type = type;
	mountpoints[num].open_callback = open_callback;
	mountpoints[num].read_callback = read_callback;
	mountpoints[num].getdents_callback = getdents_callback;

	log(LOG_DEBUG, "vfs: Mounted %s (type %s) to %s\n", dev, type, path);

	return 0;
}
