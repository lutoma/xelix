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
#include <filesystems/memfs/interface.h>

fsNode_t* vfs_createNode(char name[128], uint32 mask, uint32 uid, uint32 gid, uint32 flags, uint32 inode, uint32 length, uint32 impl, read_type_t read, write_type_t write, open_type_t open, close_type_t close, readdir_type_t readdir, finddir_type_t finddir, fsNode_t *ptr, fsNode_t *parent)
{
	fsNode_t* node = (fsNode_t*)kmalloc(sizeof(fsNode_t));
	strcpy(node->name, name);
	
	if(parent == NULL)
			parent = node; // This node is it's own parent

	log("vfs: Creating new node %s (flags 0x%x, parent %s).\n", name, flags, parent->name);
	
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
void vfs_init(multiboot_module_t mod)
{
	// Initialise the root directory.
	vfs_rootNodeCount = 0;
	vfs_rootNode = vfs_createNode("root", 0, 0, 0, FS_DIRECTORY, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL); // RootNode is it's own parent, therefore NULL as last parameter.
	
	// Load the initrd
	memfs_init(mod);
}

