/* xsfs.c: The most laughably simple 'file system' you will ever have seen. For debugging.
 * Copyright © 2016 Lukas Martini
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

#include <lib/generic.h>
#include <lib/log.h>
#include <lib/string.h>
#include <lib/md5.h>
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <fs/vfs.h>
#include "xsfs.h"

static uint32_t num_files = 0;
static uint32_t hdr_offset = 0;
static uint32_t header_end = 0;
static char* superblock = NULL;

#define read_sector_or_fail(rc, args...) do {													\
		if(ide_read_sector(args) != true) {														\
			log(LOG_ERR, "xsfs: IDE read failed in %s line %d, bailing.\n", __func__, __LINE__);	\
			return rc;																			\
		}																						\
	} while(0)

// The public readdir interface to the virtual file system
char* xsfs_read_directory(char* path, uint32_t offset)
{
	// We don't support subdirectories.
	if(strcmp("/", path) || offset >= num_files) {
		return NULL;
	}

	// Find relevant part of superblock
	char* rd = superblock + hdr_offset;

	for(int i = 0; i < offset; i++) {
		rd += find_substr(rd, ":") + 1;
	}

	// Chop off rest
	int length = find_substr(rd, "-");
	return strndup(rd, length);
}

// The public read interface to the virtual file system
size_t xsfs_read_file(void* dest, size_t size, char* path, uint32_t offset)
{
	if(path[0] == '/') {
		path++;
	}

	// Find relevant part of superblock
	char* rd = superblock + hdr_offset;
	uint32_t fileoffset = 0;
	bool found = false;
	uint32_t filesize = 0;

	for(int i = 0; i < num_files; i++) {
		int sizeidx = find_substr(rd, "-");
		int endidx = find_substr(rd + sizeidx, ":") + 1;

		char* filesize_str = strndup(rd +sizeidx + 1, endidx - 2);
		filesize = atoi(filesize_str);

		if(!strncmp(rd, path, sizeidx - 1)) {
			found = true;
			break;
		}

		fileoffset += filesize;
		rd += sizeidx + endidx;
	}

	if(!found) {
		return NULL;
	}


	if(size > filesize) {
		size = filesize;
	}

	// This is all embarassingly stupid and inefficient. But it works for now.
	char* data = kmalloc(header_end + fileoffset + filesize + 512);

	// FIXME Only read the relevant sectors
	for(int i = 0; i*512 < header_end + fileoffset + filesize + 510; i++) {
		read_sector_or_fail(NULL, 0x1F0, 0, i, (uint8_t*)(data + (i * 512)));
	}

	data += header_end + fileoffset + 1;
	data[filesize] = 0;

	memcpy(dest, data, size);

	#ifdef XSFS_MD5SUM_ALL
	log(LOG_DEBUG, "Read file %s size %d with resulting md5sum of:\n\t", path, filesize);
	MD5_dump(data, filesize);
	#endif

	return size;
}

void xsfs_init()
{
	superblock = (char*)kmalloc(512);
	read_sector_or_fail(, 0x1F0, 0, 0, (uint8_t*)superblock);

	if(memcmp(superblock, "xsfs:", 5)) {
		log(LOG_DEBUG, "This does not look like XSFS.\n");
		return;
	}

	int idx = find_substr(superblock + 5, ":");
	char* numstr = strndup(superblock + 5, idx);

	hdr_offset = 6 + idx;
	num_files = atoi(numstr);
	header_end = find_substr(superblock, "\t");

	vfs_mount("/", xsfs_read_file, xsfs_read_directory);
}
