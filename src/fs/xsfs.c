/* xsfs.c: Xelix simple file system
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

#include "xsfs.h"
#include <lib/generic.h>
#include <lib/log.h>
#include <hw/ata.h>
#include <memory/kmalloc.h>
#include <lib/print.h>
#include <lib/datetime.h>
#include <lib/string.h>

// Increase by one as soon as you change anything without legacy support
#define XSFS_VERSION 3

#define MAGIC "XSFS*"
#define BUFSIZE 256

struct header {
	char magic[5];
	uint32_t version;
	uint32_t num_files;
	uint32_t fileoffset;
} __attribute__((packed));

struct file {
	char name[50];
	uint32_t offset;
	uint32_t size;
} __attribute__((packed));

// This is so unbelievably ineffective and dumb, i'm really sorry. --Lukas
void* xsfs_read(char* path)
{
	uint16_t* buffer = (uint16_t*)kmalloc(sizeof(uint16_t) * BUFSIZE);
	ata_read(ATA0, 0, BUFSIZE - 1, buffer);
	struct header* header = (struct header*)buffer;	

	// Check magic
	if(memcmp(header->magic, MAGIC, 5) != 0)
	{
		log(LOG_ERR, "xsfs: Invalid magic '%d%d%d%d%d'\n",
		  header->magic[0],
		  header->magic[1],
		  header->magic[2],
		  header->magic[3],
		  header->magic[4]
		);
		return NULL;
	}

	if(header->num_files < 1)
	{
		log(LOG_ERR, "xsfs: Filesystem doesn't contain any files\n");
		return NULL;
	}

	if(header->version != XSFS_VERSION)
	{
		log(LOG_ERR, "xsfs: Incompatible version\n");
		return NULL;
	}

	struct file* current_file;
	for(int i = 0; i < header->num_files; i++)
	{
		current_file = (struct file*)((uint32_t)buffer + (uint32_t)header->fileoffset + (i * sizeof(struct file)));
		if(!strcmp(current_file->name, path))
			return (void*)((uint32_t)buffer + current_file->offset);
	}

	return NULL;
}