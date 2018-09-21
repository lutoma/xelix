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
#include <md5.h>
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <fs/ext2.h>

// The public read interface to the virtual file system
size_t ext2_read_file(vfs_file_t* fp, void* dest, size_t size) {
	if(!fp || !fp->inode) {
		log(LOG_ERR, "ext2: ext2_read_file called without fp or fp missing inode.\n");
		sc_errno = EBADF;
		return -1;
	}

	debug("ext2_read_file for %s, off %d, size %d\n", fp->mount_path, fp->offset, size);

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
		debug("ext2_read_file: Attempt to read something weird "
			"(0x%x: %s)\n", inode->mode,
			vfs_filetype_to_verbose(vfs_mode_to_filetype(inode->mode)));

		kfree(inode);
		sc_errno = EISDIR;
		return -1;
	}

	if(inode->size < 1) {
		kfree(inode);
		return 0;
	}

	if(size > inode->size) {
		debug("ext2_read_file: Attempt to read 0x%x bytes, but file is only 0x%x bytes. Capping.\n", size, inode->size);
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

	/* This should copy directly to dest, however read_inode_blocks can only read
	 * whole blocks right now, which means we could write more than size if size
	 * is not mod the block size. Should rewrite read_inode_blocks.
	 */
	uint8_t* tmp = kmalloc(num_blocks * superblock_to_blocksize(superblock));
	uint8_t* read = read_inode_blocks(inode, num_blocks, tmp);
	kfree(inode);

	if(!read) {
		kfree(tmp);
		return 0;
	}

	memcpy(dest, tmp, size);
	kfree(tmp);

	#ifdef EXT2_DEBUG
		log(LOG_DEBUG, "Read file %s offset %d size %d with resulting md5sum of:\n\t", fp->mount_path, fp->offset, size);
		MD5_dump(dest, size);
	#endif

	return size;
}

#endif /* ENABLE_EXT2 */
