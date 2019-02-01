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
#include <time.h>
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <fs/block.h>
#include <fs/ext2.h>

struct blocknum_resolver_cache {
	uint32_t* indirect_table;
	uint32_t* double_table;
	uint32_t double_second_block;
	uint32_t* double_second_table;
};


static uint32_t find_inode(uint32_t inode_num) {
	uint32_t blockgroup_num = inode_to_blockgroup(inode_num);
	debug("Reading inode struct %d in blockgroup %d\n", inode_num, blockgroup_num);

	// Sanity check the blockgroup num
	if(blockgroup_num > (superblock->block_count / superblock->blocks_per_group))
		return 0;

	struct blockgroup* blockgroup = blockgroup_table + blockgroup_num;
	if(!blockgroup || !blockgroup->inode_table) {
		log(LOG_ERR, "ext2: Could not locate entry %d in blockgroup table\n", blockgroup_num);
		return 0;
	}

	return bl_off(blockgroup->inode_table)
		+ ((inode_num - 1) % superblock->inodes_per_group * superblock->inode_size);
}

bool ext2_inode_read(struct inode* buf, uint32_t inode_num) {
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

bool ext2_inode_write(struct inode* buf, uint32_t inode_num) {
	if(inode_num == ROOT_INODE && root_inode) {
		memcpy(root_inode, buf, superblock->inode_size);
	}

	uint32_t inode_off = find_inode(inode_num);
	if(!inode_off) {
		return false;
	}

	return vfs_block_write(inode_off, superblock->inode_size, (uint8_t*)buf);
}

uint32_t ext2_inode_new(struct inode* inode, uint16_t mode) {
	struct blockgroup* blockgroup = blockgroup_table;
	while(!blockgroup->free_inodes) { blockgroup++; }
	uint32_t inode_num = ext2_bitmap_search_and_claim(blockgroup->inode_bitmap);

	bzero(inode, sizeof(struct inode));
	inode->link_count = 1;
	inode->mode = mode;

	uint32_t t = time_get();
	inode->creation_time = t;
	inode->modification_time = t;
	inode->access_time = t;
	ext2_inode_write(inode, inode_num);

	superblock->free_inodes--;
	blockgroup->free_inodes--;
	write_superblock();
	write_blockgroup_table();
	return inode_num;
}

static inline void free_blocknum_resolver_cache(struct blocknum_resolver_cache* cache) {
	if(cache->indirect_table) {
		kfree(cache->indirect_table);
	}

	if(cache->double_table) {
		kfree(cache->double_table);
	}

	if(cache->double_second_table) {
		kfree(cache->double_second_table);
	}

	kfree(cache);
}

// FIXME This is a mess (and also has no triply-indirect block support).
static uint32_t resolve_blocknum(struct inode* inode, uint32_t block_num, struct blocknum_resolver_cache* cache) {
	uint32_t real_block_num = 0;

	if(block_num > superblock->block_count) {
		debug("inode_resolve_blocknum: Invalid block_num (%d > %d)\n", block_num, superblock->block_count);
		return 0;
	}

	// FIXME This value (268) actually depends on block size. This expects a 1024 block size
	if(block_num >= 12 && block_num < 268) {
		if(!cache->indirect_table) {
			cache->indirect_table = (uint32_t*)kmalloc(bl_off(1));
			vfs_block_read(bl_off(inode->blocks[12]), bl_off(1), (uint8_t*)cache->indirect_table);
		}

		real_block_num = cache->indirect_table[block_num - 12];
	} else if(block_num >= 268 && block_num < 12 + 256*256) {
		if(!cache->double_table) {
			cache->double_table = (uint32_t*)kmalloc(bl_off(1));
			vfs_block_read(bl_off(inode->blocks[13]), bl_off(1), (uint8_t*)cache->double_table);
		}

		uint32_t indir_block_num = cache->double_table[(block_num - 268) / 256];

		if(!cache->double_second_table) {
			cache->double_second_table = (uint32_t*)kmalloc(bl_off(1));
		}

		if(cache->double_second_block != indir_block_num) {
			vfs_block_read(bl_off(indir_block_num), bl_off(1), (uint8_t*)cache->double_second_table);
			cache->double_second_block = indir_block_num;
		}

		real_block_num = cache->double_second_table[(block_num - 268) % 256];
	} else {
		real_block_num = inode->blocks[block_num];
	}

	debug("inode_resolve_blocknum: Translated inode block %d to real block %d\n", block_num, real_block_num);
	return real_block_num;
}

uint8_t* ext2_inode_read_data(struct inode* inode, uint32_t offset, size_t length, uint8_t* buf) {
	uint32_t num_blocks = bl_size(length);
	if(bl_mod(length) != 0) {
		num_blocks++;
	}

	// TODO Reuse offset handling from below.
	uint8_t* tmp = kmalloc(bl_off(num_blocks));
	struct blocknum_resolver_cache* res_cache = zmalloc(sizeof(struct blocknum_resolver_cache));

	for(int i = 0; i < num_blocks; i++) {
		uint32_t block_num = resolve_blocknum(inode, i + bl_size(offset), res_cache);

		if(!block_num) {
			buf = NULL;
			goto bye;
		}

		if(!vfs_block_read(bl_off(block_num), bl_off(1), tmp + bl_off(i))) {
			debug("read_inode_blocks: read for block %d failed\n", i);
			buf = NULL;
			goto bye;
		}
	}

	memcpy(buf, tmp + bl_mod(offset), length);

bye:
	free_blocknum_resolver_cache(res_cache);
	kfree(tmp);
	return buf;
}

uint8_t* ext2_inode_write_data(struct inode* inode, uint32_t inode_num, uint32_t offset, size_t length, uint8_t* buf) {
	uint32_t num_blocks = bl_size(length + offset);
	if(bl_mod(length + offset) != 0) {
		num_blocks++;
	}

	uint32_t buf_offset = 0;
	struct blocknum_resolver_cache* res_cache = zmalloc(sizeof(struct blocknum_resolver_cache));
	for(int i = 0; i < num_blocks; i++) {
		uint32_t block_num = resolve_blocknum(inode, i + bl_size(offset), res_cache);

		if(!block_num) {
			block_num = ext2_block_new(inode_num);
			if(!block_num) {
				free_blocknum_resolver_cache(res_cache);
				return NULL;
			}

			// Counts 512-byte ide blocks, not ext2 blocks, so 2.
			inode->block_count += 2;

			if(i < 12) {
				inode->blocks[i] = block_num;
			} else {
				// TODO
				log(LOG_ERR, "ext2: Indirect block writes not supported atm.\n");
				free_blocknum_resolver_cache(res_cache);
				return NULL;
			}

			ext2_inode_write(inode, inode_num);
		}

		uint32_t wr_offset = bl_off(block_num);
		uint32_t wr_size = bl_off(1);

		// Handle remainder of offset if first block
		if(i == 0) {
			wr_offset += bl_mod(offset);
			wr_size -= bl_mod(offset);
		}

		if(wr_size > length - buf_offset) {
			wr_size = length - buf_offset;
		}

		if(!vfs_block_write(wr_offset, wr_size, buf + buf_offset)) {
			debug("read_inode_blocks: write for block %d failed\n", i);
			free_blocknum_resolver_cache(res_cache);
			return NULL;
		}

		buf_offset += wr_size;
	}

	free_blocknum_resolver_cache(res_cache);
	return buf;
}

#endif /* ENABLE_EXT2 */
