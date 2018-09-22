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
#include <fs/block.h>


int ext2_stat(vfs_file_t* fp, vfs_stat_t* dest) {
	if(!fp || !fp->inode) {
		log(LOG_ERR, "ext2: ext2_read_file called without fp or fp missing inode.\n");
		return -1;
	}

	struct inode* inode = kmalloc(superblock->inode_size);
	if(!ext2_read_inode(inode, fp->inode)) {
		kfree(inode);
		return -1;
	}

	dest->st_dev = 1;
	dest->st_ino = fp->inode;
	dest->st_mode = inode->mode;
	dest->st_nlink = 0;
	dest->st_uid = inode->uid;
	dest->st_gid = inode->gid;
	dest->st_rdev = 0;
	dest->st_size = inode->size;
	dest->st_atime = inode->access_time;
	dest->st_mtime = inode->modification_time;
	dest->st_ctime = inode->creation_time;

	kfree(inode);
	return 0;
}

void ext2_init() {
	// The superblock always has an offset of 1024, so is in sector 2 & 3
	superblock = (struct superblock*)kmalloc(1024);
	read_sector_or_fail(, 0x1F0, 0, 2, (uint8_t*)superblock);
	read_sector_or_fail(, 0x1F0, 0, 3, (uint8_t*)((void*)superblock + 512));

	if(superblock->magic != SUPERBLOCK_MAGIC)
	{
		log(LOG_ERR, "ext2: Invalid magic\n");
		return;
	}

	log(LOG_INFO, "ext2: Have ext2 revision %d. %d free / %d blocks.\n",
			superblock->revision, superblock->free_blocks,
			superblock->block_count);


	// Check if the file system is marked as clean
	if(superblock->state != SUPERBLOCK_STATE_CLEAN)
	{
		log(LOG_ERR, "ext2: File system is not marked as clean.\n"
			"Please run fsck.ext2 on it.\n");
		return;
	}

	/*if(!superblock_to_blocksize(superblock) != 1024) {
		log(LOG_ERR, "ext2: Block sizes != 1024 are not supported right now.\n");
		return;
	}*/

	// TODO Compare superblocks to each other?

	// RO is irrelevant for now since we're read-only anyways.
	//if(superblock->features_incompat || superblock->features_ro)
	if(superblock->features_incompat)
	{
		log(LOG_WARN, "ext2: This filesystem uses some extensions "
			"which we don't support (incompat: 0x%x, ro: 0x%x)\n",
			superblock->features_incompat, superblock->features_ro);
		//return;
	}

	if(superblock->features_compat)
	{
		log(LOG_INFO, "ext2: This file system supports additional special "
			"features. We'll ignore them (0x%x).\n", superblock->features_compat);
	}

	debug("Loaded ext2 superblock. inode_count=%d, block_count=%d, block_size=%d\n",
		superblock->inode_count, superblock->block_count,
		superblock_to_blocksize(superblock));

	/* The number of blocks occupied by the blockgroup table
	 * There doesn't seem to be a way to directly get the number of blockgroups,
	 * so figure it out by dividing block count with blocks per group. Multiply
	 * with struct size to get total space required, then divide by block size
	 * to get ext2 blocks. Add 1 since partially used blocks also need to be
	 * allocated.
	 */
	uint32_t num_table_blocks = superblock->block_count
		/ superblock->blocks_per_group
		* sizeof(struct blockgroup)
		/ superblock_to_blocksize(superblock)
		+ 1;

	blockgroup_table = kmalloc(superblock_to_blocksize(superblock)
		* num_table_blocks);

	if(!vfs_block_read(bl_off(2), bl_off(num_table_blocks), (uint8_t*)blockgroup_table)) {
		kfree(superblock);
		kfree(blockgroup_table);
		return;
	}

	// Cache root inode
	struct inode* root_inode_buf = kmalloc(superblock->inode_size);
	if(!ext2_read_inode(root_inode_buf, ROOT_INODE)) {
		log(LOG_ERR, "ext2: Could not read root inode.\n");
		kfree(superblock);
		kfree(root_inode_buf);
		kfree(blockgroup_table);
		return;
	}

	root_inode = root_inode_buf;
	vfs_mount("/", NULL, "/dev/ide1", "ext2", ext2_open, ext2_stat,
		ext2_read_file, ext2_write_file, ext2_getdents);
}

#endif /* ENABLE_EXT2 */
