/* memfs.c: 'Driver' for memfs.
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include "interface.h"

#include <lib/log.h>
#include <filesystems/vfs.h>
#include <lib/string.h>
#include <memory/kmalloc.h>

typedef struct
{
	uint32_t fileCount; // The number of files in the ramdisk.
} memfsHeader_t;

typedef struct
{
	uint8_t magic;   // Magic number, for error checking.
	char name[64];// Filename.
	uint32_t offset; // Offset in the initrd that the file starts.
	uint32_t length; // Length of the file.
} memfsFileHeader_t;


memfsHeader_t *memfsHeader; // The header.
memfsFileHeader_t *memfsHeaders; // The list of file headers.
struct dirent dirent;

// Read single file
static size_t memfs_read(fsNode_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
	memfsFileHeader_t header = memfsHeaders[node->inode];
	if (offset > header.length)
		return 0;
	if (offset+size > header.length)
		size = header.length-offset;
	memcpy(buffer, (uint8_t*) (header.offset+offset), size);
	return size;
}

// Read directory [aka get content]
static struct dirent *memfs_readDir(fsNode_t *node, uint32_t index)
{
	if (index >= vfs_rootNodeCount)
		return 0;
	strcpy(dirent.name, vfs_rootNodes[index]->name);
	dirent.name[strlen(vfs_rootNodes[index]->name)] = 0; // Make sure the string is NULL-terminated.
	dirent.ino = vfs_rootNodes[index]->inode;
	return &dirent;

}

static fsNode_t *memfs_findDir(fsNode_t *node, char *name)
{
	int i;
	for (i = 0; i < vfs_rootNodeCount; i++)
		if(!strcmp(name, vfs_rootNodes[i]->name))
			return vfs_rootNodes[i];
	return NULL;
}

fsNode_t *memfs_init(multiboot_module_t mod)
{
	
	log("memfs: Initializing at 0x%x\n", mod.start);

	// Initialise the main and file header pointers and populate the root directory.
	memfsHeader = (memfsHeader_t *)mod.start;
	memfsHeaders = (memfsFileHeader_t *) (mod.start + sizeof(memfsHeader_t));

	if(memfsHeaders->magic != 0xBF)
		panic("Corrupt/invalid initrd (Magic != 0xBF)");

	vfs_rootNode->read = &memfs_read;
	vfs_rootNode->readDir = &memfs_readDir;
	vfs_rootNode->findDir = &memfs_findDir;
	
	vfs_rootNodeCount = memfsHeader->fileCount;
	
	vfs_rootNodes = (fsNode_t**)kmalloc(sizeof(fsNode_t) * memfsHeader->fileCount);
	log("memfs: Creating files.\n");
	
	int i;
	for (i = 0; i < memfsHeader->fileCount; i++)
	{
		// Edit the file's header - currently it holds the file offset
		// relative to the start of the ramdisk. We want it relative to the start
		// of memory.
		memfsHeaders[i].offset += mod.start;
		vfs_rootNodes[i] = vfs_createNode(memfsHeaders[i].name, 0, 0, 0, FS_FILE, i, memfsHeaders[i].length, 0, &memfs_read, NULL, NULL, NULL, NULL, NULL, NULL, vfs_rootNode);
		vfs_rootNodes[i]->read = &memfs_read;
		printf("memfs_read: 0x%x\n", vfs_rootNodes[i]->read);
		printf("vfs_rootNodes[i]: 0x%x\n", vfs_rootNodes[i]);
	}

	return NULL;
}
