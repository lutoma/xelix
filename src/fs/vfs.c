/* vfs.c: Provices abstraction from the filesystem drivers
 * Copyright © 2010, 2011 Lukas Martini
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

vfs_node_t* vfs_create_node(char name[128], uint32_t mask, uint32_t uid, uint32_t gid, uint32_t flags, uint32_t inode, uint32_t length, uint32_t impl, read_type_t read, write_type_t write, open_type_t open, close_type_t close, readdir_type_t readdir, finddir_type_t finddir, vfs_node_t* ptr, vfs_node_t* parent)
{
	vfs_node_t* node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
	strcpy(node->name, name);
	
	if(parent == NULL)
			parent = node; // This node is it's own parent

	log(LOG_INFO, "vfs: Creating new node %s (flags 0x%x, parent %s).\n", name, flags, parent->name);
	
	node->mask = mask;
	node->uid = uid;
	node->gid = gid;
	node->flags = flags;
	node->inode = inode;
	node->length = length;
	node->impl = impl;
	node->read = read;
	node->write = write;
	node->open = open;
	node->close = close;
	node->readdir = readdir;
	node->finddir = finddir;
	node->ptr = ptr;
	node->parent = parent;

	return node;
}

// Initialize the filesystem abstraction system
void vfs_init()
{
	// Initialise the root directory.
	vfs_rootnodecount = 0;
	vfs_rootnode = vfs_create_node("root", 0, 0, 0, FS_DIRECTORY, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL); // RootNode is it's own parent, therefore NULL as last parameter.
}

