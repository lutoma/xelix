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

#define MAX_MOUNTPOINTS 50
#define MAX_OPENFILES 500

struct mountpoint
{
	char path[265];
	bool active;
	vfs_read_callback_t read_callback;
	vfs_read_dir_callback_t read_dir_callback;
};

struct mountpoint mountpoints[MAX_MOUNTPOINTS];
vfs_file_t files[MAX_OPENFILES];
vfs_dir_t dirs[MAX_OPENFILES];
uint32_t last_mountpoint = -1;
uint32_t last_file = -1;
uint32_t last_dir = -1;
static spinlock_t file_open_lock;

// Debugging function – Dump a directory and all subdirectories
void vfs_dump_dir(char* path) {
	log(LOG_DEBUG, "Files in %s\n", path);
	vfs_dir_t* dir = vfs_dir_open(path);
	if(!dir) {
		log(LOG_ERR, "vfs_dump_dir: Could not open directory %s\n", path);
		return;
	}

	char* name = vfs_dir_read(dir, 0);
	for(int i = 1; name; i++) {
		log(LOG_DEBUG, "%s\n", name);
		name = vfs_dir_read(dir, i);
	}
}

vfs_file_t* vfs_get_from_id(uint32_t id)
{
	if(id < 1 || id > last_file || !spinlock_get(&file_open_lock, 30)) {
		return NULL;
	}

	vfs_file_t* fd = &files[id];
	spinlock_release(&file_open_lock);

	return fd;
}

vfs_dir_t* vfs_get_dir_from_id(uint32_t id)
{
	if(id < 1 || id > last_dir || !spinlock_get(&file_open_lock, 30)) {
		return NULL;
	}

	vfs_dir_t* dd = &dirs[id];
	spinlock_release(&file_open_lock);

	return dd;
}


size_t vfs_read(void* dest, size_t size, vfs_file_t* fp)
{
	strncpy(vfs_last_read_attempt, fp->path, 512);

	struct mountpoint mp = mountpoints[fp->mountpoint];
	if(!mp.read_callback)
		return NULL;

	size_t read = mp.read_callback(dest, size, fp->mount_path, fp->offset);
	fp->offset += size;

	return read;
}

char* vfs_dir_read(vfs_dir_t* dir, uint32_t offset)
{
	strncpy(vfs_last_read_attempt, dir->path, 512);

	struct mountpoint mp = mountpoints[dir->mountpoint];
	if(!mp.read_dir_callback)
		return NULL;
	
	char* name = mp.read_dir_callback (dir->mount_path, offset);
	if(name)
		return name;

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
	if(!spinlock_get(&file_open_lock, 30)) {
		return NULL;
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

vfs_dir_t* vfs_dir_open(char* path)
{
	uint32_t num;
	spinlock_cmd(num = ++last_dir, 20, (vfs_dir_t*)-1);

	dirs[num].num = num;
	strcpy(dirs[num].path, path);
	strcpy(dirs[num].mount_path, path); // Fixme
	dirs[num].mountpoint = 0; // Fixme
	return &dirs[num];
}

int vfs_mount(char* path, vfs_read_callback_t read_callback, vfs_read_dir_callback_t read_dir_callback)
{
	uint32_t num;
	spinlock_cmd(num = ++last_mountpoint, 20, -1);

	strcpy(mountpoints[num].path, path);
	mountpoints[num].active = true;
	mountpoints[num].read_callback = read_callback;
	mountpoints[num].read_dir_callback = read_dir_callback;

	log(LOG_DEBUG, "Mounted [%x] to %s\n", read_callback, mountpoints[num].path);
	return 0;
}
