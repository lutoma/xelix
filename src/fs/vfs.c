/* vfs.c: Virtual file system
 * Copyright © 2011-2016 Lukas Martini
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

// TODO: See doc/vfs.txt

#include "vfs.h"

#include <lib/log.h>
#include <memory/kmalloc.h>
#include <lib/string.h>
#include <lib/list.h>
#include <lib/spinlock.h>


struct mountpoint
{
	char path[265];
	bool active;
	vfs_open_callback_t open_callback;
	vfs_read_callback_t read_callback;
	vfs_read_dir_callback_t read_dir_callback;
};

struct mountpoint mountpoints[VFS_MAX_MOUNTPOINTS];
vfs_file_t files[VFS_MAX_OPENFILES];
uint32_t last_mountpoint = -1;
uint32_t last_file = -1;
uint32_t last_dir = 0;
static spinlock_t file_open_lock;


vfs_file_t* vfs_get_from_id(uint32_t id)
{
	if(id < 1 || id > last_file || !spinlock_get(&file_open_lock, 30)) {
		return NULL;
	}

	vfs_file_t* fd = &files[id];
	spinlock_release(&file_open_lock);

	return fd;
}


size_t vfs_read(void* dest, size_t size, vfs_file_t* fp)
{
	strncpy(vfs_last_read_attempt, fp->path, 512);

	struct mountpoint mp = mountpoints[fp->mountpoint];
	if(!mp.read_callback)
		return 0;

	size_t read = mp.read_callback(dest, size, fp->mount_path, fp->offset);
	fp->offset += size;

	return read;
}

char* vfs_dir_read(vfs_file_t* dir)
{
	strncpy(vfs_last_read_attempt, dir->path, 512);

	struct mountpoint mp = mountpoints[dir->mountpoint];
	if(!mp.read_dir_callback)
		return NULL;

	char* name = mp.read_dir_callback (dir->mount_path, dir->offset);
	if(name) {
		dir->offset++;
		return name;
	}

	return NULL;
}

void vfs_seek(vfs_file_t* fp, uint32_t offset, int origin)
{
	switch(origin)
	{
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

vfs_file_t* vfs_open(char* path)
{
	if(!path || !strcmp(path, "")) {
		log(LOG_ERR, "vfs: vfs_open called with empty path.\n");
		return NULL;
	}

	if(!spinlock_get(&file_open_lock, 30)) {
		return NULL;
	}

	printf("vfs_open for path \"%s\"\n", path);

	struct mountpoint mp = mountpoints[0]; // Fixme
	if(!mp.open_callback) {
		log(LOG_WARN, "vfs: fs has no open callback, can not check file existence\n");
	} else {
		if(mp.open_callback(path)) {
			spinlock_release(&file_open_lock);
			return NULL;
		}
	}

	uint32_t num = (last_file == -1 ? last_file = 3 : ++last_file);

	files[num].num = num;
	strcpy(files[num].path, path);
	strcpy(files[num].mount_path, path); // Fixme
	files[num].offset = 0;
	files[num].mountpoint = 0; // Fixme

	spinlock_release(&file_open_lock);
	return &files[num];
}

int vfs_mount(char* path,  vfs_open_callback_t open_callback,
	vfs_read_callback_t read_callback, vfs_read_dir_callback_t read_dir_callback) {

	if(!path || !strcmp(path, "")) {
		log(LOG_ERR, "vfs: vfs_mount called with empty path.\n");
		return NULL;
	}

	uint32_t num;
	spinlock_cmd(num = ++last_mountpoint, 20, -1);

	strcpy(mountpoints[num].path, path);
	mountpoints[num].active = true;
	mountpoints[num].open_callback = open_callback;
	mountpoints[num].read_callback = read_callback;
	mountpoints[num].read_dir_callback = read_dir_callback;

	log(LOG_DEBUG, "Mounted [%x] to %s\n", read_callback, mountpoints[num].path);
	return 0;
}
