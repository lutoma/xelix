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
#include <time.h>
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <fs/block.h>
#include <fs/ext2.h>


static uint32_t find_inode(uint32_t inode_num) {
	uint32_t blockgroup_num = inode_to_blockgroup(inode_num);
	debug("Reading inode struct %d in blockgroup %d\n", inode_num, blockgroup_num);

	// Sanity check the blockgroup num
	if(blockgroup_num > (superblock->block_count / superblock->blocks_per_group))
		return 0;

	struct blockgroup* blockgroup = blockgroup_table + blockgroup_num;
	if(!blockgroup || !blockgroup->inode_table) {
		debug("Could not locate entry %d in blockgroup table\n", blockgroup_num);
		return 0;
	}

	return bl_off(blockgroup->inode_table)
		+ ((inode_num - 1) % superblock->inodes_per_group * superblock->inode_size);
}

bool ext2_read_inode(struct inode* buf, uint32_t inode_num) {
	if(inode_num == ROOT_INODE && root_inode) {
		memcpy(buf, root_inode, superblock->inode_size);
		return true;
	}

	uint32_t inode_off = find_inode(inode_num);
	if(!inode_off) {
		return false;
	}

	return vfs_block_read(inode_off, superblock->inode_size, (uint8_t*)buf);
}

bool ext2_write_inode(struct inode* buf, uint32_t inode_num) {
	if(inode_num == ROOT_INODE && root_inode) {
		memcpy(root_inode, buf, superblock->inode_size);
	}

	uint32_t inode_off = find_inode(inode_num);
	if(!inode_off) {
		return false;
	}

	return vfs_block_write(inode_off, superblock->inode_size, (uint8_t*)buf);
}

uint32_t ext2_new_inode(struct inode** inodeptr) {
	struct blockgroup* blockgroup = blockgroup_table;
	while(!blockgroup->free_inodes) { blockgroup++; }
	uint32_t inode_num = ext2_bitmap_search_and_claim(blockgroup->inode_bitmap);

	*inodeptr = kmalloc(superblock->inode_size);
	bzero(*inodeptr, sizeof(struct inode));
	(*inodeptr)->link_count = 1;
	(*inodeptr)->mode = FT_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	uint32_t t = time_get();
	(*inodeptr)->creation_time = t;
	(*inodeptr)->modification_time = t;
	(*inodeptr)->access_time = t;
	ext2_write_inode(*inodeptr, inode_num);

	// TODO Also decrement blockgroup->free_inodes
	superblock->free_inodes--;
	write_superblock();

	return inode_num;
}

// FIXME This is a mess (and also has no triply-indirect block support).
uint32_t ext2_resolve_inode_blocknum(struct inode* inode, uint32_t block_num) {
	uint32_t real_block_num = 0;

	if(block_num > superblock->block_count) {
		debug("resolve_inode_blocknum: Invalid block_num (%d > %d)\n", block_num, superblock->block_count);
		return 0;
	}

	// FIXME This value (268) actually depends on block size. This expects a 1024 block size
	if(block_num >= 12 && block_num < 268) {
		debug("reading indirect block at 0x%x\n", inode->blocks[12]);

		// Indirect block
		uint32_t* blocks_table = (uint32_t*)kmalloc(bl_off(1));
		vfs_block_read(bl_off(inode->blocks[12]), bl_off(1), (uint8_t*)blocks_table);
		real_block_num = blocks_table[block_num - 12];
		kfree(blocks_table);
	} else if(block_num >= 268 && block_num < 12 + 256*256) {
		uint32_t* blocks_table = (uint32_t*)kmalloc(bl_off(1));
		vfs_block_read(bl_off(inode->blocks[13]), bl_off(1), (uint8_t*)blocks_table);
		uint32_t indir_block_num = blocks_table[(block_num - 268) / 256];

		uint32_t* indir_blocks_table = (uint32_t*)kmalloc(bl_off(1));
		vfs_block_read(bl_off(indir_block_num), bl_off(1), (uint8_t*)indir_blocks_table);
		real_block_num = indir_blocks_table[(block_num - 268) % 256];
		kfree(blocks_table);
		kfree(indir_blocks_table);
	} else {
		real_block_num = inode->blocks[block_num];
	}

	debug("resolve_inode_blocknum: Translated inode block %d to real block %d\n", block_num, real_block_num);
	return real_block_num;
}

uint8_t* ext2_read_inode_blocks(struct inode* inode, uint32_t num, uint8_t* buf) {
	for(int i = 0; i < num; i++) {
		uint32_t block_num = ext2_resolve_inode_blocknum(inode, i);
		if(!block_num) {
			return NULL;
		}

		if(!vfs_block_read(bl_off(block_num), bl_off(1), buf + bl_off(i))) {
			debug("read_inode_blocks: read for block %d failed\n", i);
			return NULL;
		}
	}

	return buf;
}

int ext2_write_inode_blocks(struct inode* inode, uint32_t inode_num, uint32_t num, uint8_t* buf) {
	for(int i = 0; i < num; i++)
	{
		uint32_t block_num = ext2_resolve_inode_blocknum(inode, i);
		if(!block_num) {
			block_num = ext2_new_block(inode_num);
			if(!block_num) {
				return 0;
			}

			// Counts 512-byte ide blocks, not ext2 blocks, so 2.
			inode->block_count += 2;
		}

		if(!vfs_block_write(bl_off(block_num), bl_off(1), buf + bl_off(i))) {
			debug("write_inode_blocks: write for block %d failed\n", i);
			return 0;
		}

		if(i < 12) {
			inode->blocks[i] = block_num;
		} else {
			// TODO
			log(LOG_ERR, "ext2: Indirect block writes not supported atm.\n");
			return 0;
		}
	}

	ext2_write_inode(inode, inode_num);
	return 1;
}

#endif /* ENABLE_EXT2 */
