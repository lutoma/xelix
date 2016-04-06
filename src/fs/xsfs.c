/* ext2.c: The most laughably simple 'file system' you will have ever seen. For debugging.
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

uint32_t file_size = 0;
uint32_t file_offset = 0;
static char* superblock = NULL;

#define read_sector_or_fail(rc, args...) do {													\
		if(ide_read_sector(args) != true) {														\
			log(LOG_ERR, "ext2: IDE read failed in %s line %d, bailing.\n", __func__, __LINE__);	\
			return rc;																			\
		}																						\
	} while(0)

// The public readdir interface to the virtual file system
char* xsfs_read_directory(char* path, uint32_t offset)
{
	// We don't support directories.
	if(path != "/" || offset != 0) {
		return NULL;
	}

	return "thefile";
}

// The public read interface to the virtual file system
void* xsfs_read_file(char* path, uint32_t offset, uint32_t size)
{
	if(strcmp(path, "/thefile") || offset != 0) {
		return 0;
	}

	char* data = kmalloc_a(file_size + 512);

	for(int i = 0; i*512 < file_size + 510; i++) {
		read_sector_or_fail(, 0x1F0, 0, i, (uint8_t*)(data + (i * 512)));
	}

	data += file_offset;
	return data;
}

void xsfs_init()
{
	superblock = (char*)kmalloc(512);
	read_sector_or_fail(, 0x1F0, 0, 0, (uint8_t*)superblock);

	if(memcmp(superblock, "xsfs:", 7)) {
		log(LOG_DEBUG, "This does not look like XSFS.\n");
		return;
	}

	int idx = find_substr(superblock + 7, ":");
	char* sizestr = strndup(superblock + 7, idx);

	file_offset = 8 + idx;
	file_size = atoi(sizestr);

	log(LOG_DEBUG, "file size seems to be: %d\n", file_size);

	vfs_mount("/", xsfs_read_file, xsfs_read_directory);
}
