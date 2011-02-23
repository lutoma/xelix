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

#include <common/log.h>
#include <memory/kmalloc.h>
#include <common/string.h>
#include <common/fio.h>
/*
 * // Is the node a directory, and does it have a callback?
 * if ( (node->flags&0x7) == FS_DIRECTORY && node->readdir != 0 )
*/
struct dirent dirent;
// Read directory [aka get content]
static struct dirent *dummyReadDir(fsNode_t* node, uint32 index)
{
	if(node != vfs_rootNode || index > 1) // Note: this is only hardcoded sh*t and should be replaced ASAP!
		return 0;

	strcpy(dirent.name, node->name);
	dirent.name[strlen(node->name)] = 0; // Make sure the string is NULL-terminated.
	dirent.ino = node->inode;
	return &dirent;
}

static fsNode_t *dummyFindDir(fsNode_t *node, char *name)
{
	return 0;
}

fsNode_t* vfs_createNode(char name[128], uint32 mask, uint32 uid, uint32 gid, uint32 flags, uint32 inode, uint32 length, uint32 impl, read_type_t read, write_type_t write, open_type_t open, close_type_t close, readdir_type_t readdir, finddir_type_t finddir, fsNode_t *ptr, fsNode_t *parent)
{
	fsNode_t* node = kmalloc(sizeof(fsNode_t));
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
void vfs_init(char** modules)
{
	// Load the initrd
	fsNode_t *bla = memfs_init(modules[0]);
}

