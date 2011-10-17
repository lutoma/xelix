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

#define MAX_MOUNTPOINTS 50

vfs_node_t mountpoints[MAX_MOUNTPOINTS];
uint32_t mountpoints_last_used = -1;

/*
vfs_storage_t* vfs_get_from_path(const char* path)
{
	// Scalability & speed? Nope, chuck testa.
	// TODO
}*/

vfs_storage_t* vfs_get_from_id(uint32_t id)
{
	// Scalability & speed? Nope, chuck testa.
	return mountpoints[id].storage;
}

void vfs_mount(const char* path, vfs_storage_t* store)
{
	uint32_t num = ++mountpoints_last_used;
	mountpoints[num].path = path;
	mountpoints[num].storage = store;
	mountpoints[num].active = true;
}

// For future use
void vfs_init(){}
