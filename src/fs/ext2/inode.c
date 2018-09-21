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
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <fs/block.h>
#include <fs/ext2.h>

bool direct_write_blocks(uint32_t block_num, uint32_t write_num, uint8_t* buf) {
	debug("direct_write_blocks, writing from block %d size %d\n", block_num, write_num);

	block_num *= superblock_to_blocksize(superblock);
	block_num /= 512;
	write_num *= superblock_to_blocksize(superblock);
	write_num /= 512;

	for(int i = 0; i < write_num; i++) {
		ide_write_sector_retry(0x1F0, 0, block_num + i, buf + (i * 512));
	}

	return true;
}

bool read_inode(struct inode* buf, uint32_t inode_num) {
	if(inode_num == ROOT_INODE && root_inode) {
		memcpy(buf, root_inode, superblock->inode_size);
		return true;
	}

	uint32_t blockgroup_num = inode_to_blockgroup(inode_num);
	debug("Reading inode struct %d in blockgroup %d\n", inode_num, blockgroup_num);

	// Sanity check the blockgroup num
	if(blockgroup_num > (superblock->block_count / superblock->blocks_per_group))
		return false;

	struct blockgroup* blockgroup = blockgroup_table + blockgroup_num;
	if(!blockgroup || !blockgroup->inode_table) {
		debug("Could not locate entry %d in blockgroup table\n", blockgroup_num);
		return false;
	}

	return vfs_block_read(bl_off(blockgroup->inode_table) +
		((inode_num - 1) % superblock->inodes_per_group * superblock->inode_size),
		superblock->inode_size, buf);
}

uint32_t resolve_inode_blocknum(struct inode* inode, uint32_t block_num) {
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
		vfs_block_read(bl_off(inode->blocks[12]), bl_off(1), blocks_table);
		real_block_num = blocks_table[block_num - 12];
		kfree(blocks_table);
	} else if(block_num >= 268 && block_num < 12 + 256*256) {
		uint32_t* blocks_table = (uint32_t*)kmalloc(bl_off(1));
		vfs_block_read(bl_off(inode->blocks[13]), bl_off(1), blocks_table);
		uint32_t indir_block_num = blocks_table[(block_num - 268) / 256];

		uint32_t* indir_blocks_table = (uint32_t*)kmalloc(bl_off(1));
		vfs_block_read(bl_off(indir_block_num), bl_off(1), indir_blocks_table);
		real_block_num = indir_blocks_table[(block_num - 268) % 256];
		kfree(blocks_table);
		kfree(indir_blocks_table);
	} else {
		real_block_num = inode->blocks[block_num];
	}

	debug("resolve_inode_blocknum: Translated inode block %d to real block %d\n", block_num, real_block_num);
	return real_block_num;
}

uint8_t* read_inode_blocks(struct inode* inode, uint32_t num, uint8_t* buf) {
	uint32_t block_size = superblock_to_blocksize(superblock);

	for(int i = 0; i < num; i++) {
		uint32_t block_num = resolve_inode_blocknum(inode, i);
		if(!block_num) {
			return NULL;
		}

		if(!vfs_block_read(block_num * block_size, block_size, buf + block_size * i)) {
			debug("read_inode_blocks: read_inode_block for block %d failed\n", i);
			return NULL;
		}
	}

	return buf;
}

int write_inode_blocks(struct inode* inode, uint32_t num, uint8_t* buf) {
	for(int i = 0; i < num; i++)
	{
		uint32_t real_block_num = resolve_inode_blocknum(inode, i);
		if(!real_block_num) {
			return 0;
		}

		if(!direct_write_blocks(real_block_num, 1, buf + superblock_to_blocksize(superblock) * i)) {
			debug("write_inode_blocks: write for block %d failed\n", i);
			return 0;
		}
	}

	return 1;
}

#endif /* ENABLE_EXT2 */
