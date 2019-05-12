/* ext2.c: Implementation of the extended file system, version 2
 * Copyright © 2013-2019 Lukas Martini
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
#include <print.h>
#include <time.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>
#include <fs/ext2.h>

uint32_t ext2_block_new(uint32_t neighbor) {
	uint32_t pref_blockgroup = inode_to_blockgroup(neighbor);

	struct blockgroup* blockgroup = blockgroup_table + pref_blockgroup;
	if(!blockgroup || !blockgroup->inode_table) {
		log(LOG_ERR, "ext2: Could not locate entry %d in blockgroup table\n", pref_blockgroup);
		return 0;
	}

	uint32_t block_num = ext2_bitmap_search_and_claim(blockgroup->block_bitmap);
	if(!block_num) {
		log(LOG_ERR, "ext2: Could not find free block in preferred blockgroup %d.\n", pref_blockgroup);
		return 0;
	}

	superblock->free_blocks--;
	blockgroup->free_blocks--;
	write_superblock();
	write_blockgroup_table();
	return block_num;

}

size_t ext2_write(vfs_file_t* fp, void* source, size_t size, task_t* task) {
	if(!fp || !fp->inode) {
		log(LOG_ERR, "ext2: ext2_write_file called without fp or fp missing inode.\n");
		sc_errno = EBADF;
		return -1;
	}

	debug("ext2_write_file for %s, off %d, size %d\n", fp->mount_path, fp->offset, size);

	struct inode* inode = kmalloc(superblock->inode_size);
	if(!ext2_inode_read(inode, fp->inode)) {
		kfree(inode);
		sc_errno = EBADF;
		return -1;
	}

	if(ext2_inode_check_perm(PERM_CHECK_WRITE, inode, task) < 0) {
		kfree(inode);
		sc_errno = EACCES;
		return -1;
	}

	debug("%s uid=%d, gid=%d, size=%d, ft=%s mode=%s\n", fp->mount_path, inode->uid,
		inode->gid, inode->size, vfs_filetype_to_verbose(vfs_mode_to_filetype(inode->mode)),
		vfs_get_verbose_permissions(inode->mode));

	if(vfs_mode_to_filetype(inode->mode) != FT_IFREG) {
		debug("ext2_write_file: Attempt to write to something weird "
			"(0x%x: %s)\n", inode->mode,
			vfs_filetype_to_verbose(vfs_mode_to_filetype(inode->mode)));

		kfree(inode);
		sc_errno = EISDIR;
		return -1;
	}

	if(!ext2_inode_write_data(inode, fp->inode, fp->offset, size, source)) {
		return -1;
	}

	inode->size = fp->offset + size;
	inode->mtime = time_get();
	ext2_inode_write(inode, fp->inode);
	kfree(inode);
	return size;
}

#endif /* ENABLE_EXT2 */
