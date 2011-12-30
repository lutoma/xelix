/* vfs.c: Virtual file system
 * Copyright © 2011 Lukas Martini
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
#include <fs/xsfs.h>

#define MAX_MOUNTPOINTS 50
#define MAX_OPENFILES 500

struct mountpoint
{
	char path[265];
	bool active;
	vfs_read_callback_t read_callback;
};

struct mountpoint mountpoints[MAX_MOUNTPOINTS];
vfs_file_t files[MAX_OPENFILES];
uint32_t last_mountpoint = -1;
uint32_t last_file = -1;

vfs_file_t* vfs_get_from_id(uint32_t id)
{
	return &files[id];
}

void* vfs_read(vfs_file_t* fp)
{
	struct mountpoint mp = mountpoints[fp->mountpoint];
	return mp.read_callback (fp->mount_path, fp->offset);
	return NULL;
}

vfs_file_t* vfs_open(char* path)
{
	uint32_t num;
	spinlock_cmd(num = ++last_file, 20, (vfs_file_t*)-1);

	files[num].num = num;
	strcpy(files[num].path, path);
	strcpy(files[num].mount_path, path); // Fixme
	files[num].offset = 0;
	files[num].mountpoint = 0; // Fixme
	return &files[num];
}

int vfs_mount(char* path, vfs_read_callback_t read_callback)
{
	uint32_t num;
	spinlock_cmd(num = ++last_mountpoint, 20, -1);

	strcpy(mountpoints[num].path, path);
	mountpoints[num].active = true;
	mountpoints[num].read_callback = read_callback;

	log(LOG_DEBUG, "Mounted [%x] to %s\n", read_callback, mountpoints[num].path);
	return 0;
}
