/* ext2.c: Implementation of the extended file system, version 2
 * Copyright © 2013-2018 Lukas Martini
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

#ifdef ENABLE_EXT2

#include "ext2_internal.h"
#include <log.h>
#include <string.h>
#include <errno.h>
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <fs/ext2.h>
#include <print.h>
/*
// TODO add neighbor param to try to find new block close to other ino blocks to avoid fragmentation
static uint32_t find_free_block() {
	printf("find_free_block\n");
}
*/
size_t ext2_write_file(vfs_file_t* fp, void* source, size_t size) {
	if(!fp || !fp->inode) {
		log(LOG_ERR, "ext2: ext2_write_file called without fp or fp missing inode.\n");
		sc_errno = EBADF;
		return -1;
	}

	debug("ext2_write_file for %s, off %d, size %d\n", fp->mount_path, fp->offset, size);
	log(LOG_DEBUG, "ext2_write_file for %s, off %d, size %d\n", fp->mount_path, fp->offset, size);

	if(fp->offset) {
		log(LOG_ERR, "ext2: Writes with offset not supported atm.\n");
		sc_errno = ENOSYS;
		return -1;
	}

	struct inode* inode = kmalloc(superblock->inode_size);
	if(!read_inode(inode, fp->inode)) {
		kfree(inode);
		sc_errno = EBADF;
		return -1;
	}

	debug("%s uid=%d, gid=%d, size=%d, ft=%s mode=%s\n", fp->mount_path, inode->uid,
		inode->gid, inode->size, vfs_filetype_to_verbose(vfs_mode_to_filetype(inode->mode)),
		vfs_get_verbose_permissions(inode->mode));

	if(vfs_mode_to_filetype(inode->mode) != FT_IFREG)
	{
		debug("ext2_write_file: Attempt to write to something weird "
			"(0x%x: %s)\n", inode->mode,
			vfs_filetype_to_verbose(vfs_mode_to_filetype(inode->mode)));

		kfree(inode);
		sc_errno = EISDIR;
		return -1;
	}

	if(size > inode->size) {
		debug("ext2_write_file: Attempt to write 0x%x bytes, but file is only 0x%x bytes. Capping.\n", size, inode->size);
		size = inode->size;
	}

	uint32_t num_blocks = (size + fp->offset) / superblock_to_blocksize(superblock);
	if((size + fp->offset) % superblock_to_blocksize(superblock) != 0) {
		num_blocks++;
	}

	debug("Inode has %d blocks:\n", num_blocks);
	debug("Blocks table:\n");
	for(uint32_t i = 0; i < 15; i++) {
		debug("\t%2d: 0x%x\n", i, inode->blocks[i]);
	}

	uint8_t* tmp = kmalloc(num_blocks * superblock_to_blocksize(superblock));
	bzero(tmp, num_blocks * superblock_to_blocksize(superblock));
	uint8_t* read = read_inode_blocks(inode, num_blocks, tmp);
	memcpy(read, source, size);
	write_inode_blocks(inode, num_blocks, read);
	kfree(inode);
	kfree(tmp);

	return 1;
}

#endif /* ENABLE_EXT2 */
