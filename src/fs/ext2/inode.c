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
#include "misc.h"
#include "inode.h"
#include <log.h>
#include <string.h>
#include <time.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>
#include <fs/block.h>
#include <fs/ext2.h>

#define INODE_CACHE_MAX 0x100
struct inode_cache_entry {
	uint32_t num;
	struct inode inode;
};
static struct inode_cache_entry inode_cache[INODE_CACHE_MAX];
static uint32_t inode_cache_end = 0;

static uint64_t find_inode(uint32_t inode_num) {
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

static struct inode* check_cache(uint32_t inode_num) {
	for(int i = 0; i < INODE_CACHE_MAX && inode_cache[i].num; i++) {
		if(inode_cache[i].num == inode_num) {
			return &inode_cache[i].inode;
		}
	}
	return NULL;
}

bool ext2_inode_read(struct inode* buf, uint32_t inode_num) {
	if(inode_num == ROOT_INODE && root_inode) {
		memcpy(buf, root_inode, superblock->inode_size);
		return true;
	}

	struct inode* cache_in = check_cache(inode_num);
	if(cache_in) {
		memcpy(buf, cache_in, superblock->inode_size);
		return true;
	}

	uint64_t inode_off = find_inode(inode_num);
	if(!inode_off) {
		return false;
	}

	if(vfs_block_sread(ext2_block_dev, inode_off, superblock->inode_size, (uint8_t*)buf) < superblock->inode_size) {
		return false;
	}

	struct inode_cache_entry cache = inode_cache[inode_cache_end];
	memcpy(&cache.inode, buf, superblock->inode_size);
	cache.num = inode_num;
	inode_cache_end++;
	inode_cache_end %= INODE_CACHE_MAX;
	return true;
}

bool ext2_inode_write(struct inode* buf, uint32_t inode_num) {
	if(inode_num == ROOT_INODE && root_inode) {
		memcpy(root_inode, buf, superblock->inode_size);
	}

	uint64_t inode_off = find_inode(inode_num);
	if(!inode_off) {
		return false;
	}

	struct inode* cache_in = check_cache(inode_num);
	if(cache_in) {
		memcpy(cache_in, buf, superblock->inode_size);
	}

	return vfs_block_swrite(ext2_block_dev, inode_off, superblock->inode_size, (uint8_t*)buf);
}

uint32_t ext2_inode_new(struct inode* inode, uint16_t mode) {
	struct blockgroup* blockgroup = blockgroup_table;
	while(!blockgroup->free_inodes) { blockgroup++; }
	uint32_t inode_num = ext2_bitmap_search_and_claim(blockgroup->inode_bitmap);

	bzero(inode, superblock->inode_size);
	inode->mode = mode;

	uint32_t t = time_get();
	inode->ctime = t;
	inode->mtime = t;
	inode->atime = t;
	ext2_inode_write(inode, inode_num);

	superblock->free_inodes--;
	blockgroup->free_inodes--;
	write_superblock();
	write_blockgroup_table();
	return inode_num;
}

void ext2_free_blocknum_resolver_cache(struct ext2_blocknum_resolver_cache* cache) {
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
uint32_t ext2_resolve_blocknum(struct inode* inode, uint32_t block_num, struct ext2_blocknum_resolver_cache* cache) {
	uint32_t real_block_num = 0;
	const uint32_t entries_per_block = bl_off(1) / sizeof(uint32_t);

	if(block_num < 12) {
		real_block_num = inode->blocks[block_num];
	} else if(block_num < entries_per_block + 12) {
		if(!inode->blocks[12]) {
			return 0;
		}

		if(!cache->indirect_table) {
			cache->indirect_table = (uint32_t*)zmalloc(bl_off(1));
			vfs_block_sread(ext2_block_dev, bl_off(inode->blocks[12]), bl_off(1), (uint8_t*)cache->indirect_table);
		}

		real_block_num = cache->indirect_table[block_num - 12];
	} else if(block_num < entries_per_block * entries_per_block + 12) {
		if(!inode->blocks[13]) {
			return 0;
		}

		if(!cache->double_table) {
			cache->double_table = (uint32_t*)zmalloc(bl_off(1));
			vfs_block_sread(ext2_block_dev, bl_off(inode->blocks[13]), bl_off(1), (uint8_t*)cache->double_table);
		}

		uint32_t indir_block_num = cache->double_table[(block_num - entries_per_block - 12) / entries_per_block];
		if(!indir_block_num) {
			return 0;
		}

		if(!cache->double_second_table) {
			cache->double_second_table = (uint32_t*)zmalloc(bl_off(1));
		}

		if(cache->double_second_block != indir_block_num) {
			vfs_block_sread(ext2_block_dev, bl_off(indir_block_num), bl_off(1), (uint8_t*)cache->double_second_table);
			cache->double_second_block = indir_block_num;
		}

		real_block_num = cache->double_second_table[(block_num - entries_per_block - 12) % entries_per_block];
	} else {
		return 0;
	}

	return real_block_num;
}

/* Will write if write_inode_num is set, otherwise read. Use
 * exta_inode_read_data/exta_inode_write_data macros instead.
 */
uint8_t* ext2_inode_data_rw(struct inode* inode, uint32_t write_inode_num,
	uint64_t offset, size_t length, uint8_t* buf) {


	uint32_t num_blocks = bl_size(length);
	if(bl_mod(length)) {
		num_blocks++;
	}
	if(bl_mod(offset) && bl_mod(offset) + bl_mod(length) > bl_off(1)) {
		num_blocks++;
	}

	uint32_t buf_offset = 0;
	struct ext2_blocknum_resolver_cache* res_cache = zmalloc(sizeof(struct ext2_blocknum_resolver_cache));
	for(int i = 0; i < num_blocks; i++) {
		uint32_t block_num = ext2_resolve_blocknum(inode, i + bl_size(offset), res_cache);

		if(!block_num) {
			if(!write_inode_num) {
				return NULL;
			}

			block_num = ext2_block_new(write_inode_num);
			if(!block_num) {
				ext2_free_blocknum_resolver_cache(res_cache);
				return NULL;
			}

			// Counts 512-byte ide blocks, not ext2 blocks, so 2.
			inode->block_count += 2;

			if(i < 12) {
				inode->blocks[i] = block_num;
			} else {
				// TODO
				log(LOG_ERR, "ext2: Indirect block writes not supported atm.\n");
				ext2_free_blocknum_resolver_cache(res_cache);
				return NULL;
			}

			ext2_inode_write(inode, write_inode_num);
		}

		uint64_t wr_offset = bl_off(block_num);
		uint64_t wr_size = bl_off(1);

		// Handle remainder of offset if first block
		if(i == 0) {
			wr_offset += bl_mod(offset);
			wr_size -= bl_mod(offset);
		}

		if(wr_size > length - buf_offset) {
			wr_size = length - buf_offset;
		}

		int nread = -1;
		if(write_inode_num) {
			nread = vfs_block_swrite(ext2_block_dev, wr_offset, wr_size, buf + buf_offset);
		} else {
			nread = vfs_block_sread(ext2_block_dev, wr_offset, wr_size, buf + buf_offset);
		}

		if(nread < wr_size) {
			ext2_free_blocknum_resolver_cache(res_cache);
			return NULL;
		}

		buf_offset += wr_size;
	}

	ext2_free_blocknum_resolver_cache(res_cache);
	return buf;
}

int ext2_inode_check_perm(enum inode_check_op op, struct inode* inode, task_t* task) {
	// Kernel / root
	if(!task || task->euid == 0) {
		return 0;
	}

	int bit_offset = 0;
	if(task->euid == inode->uid) {
		bit_offset = 6;
	} else if(task->egid == inode->gid) {
		bit_offset = 3;
	}

	if(inode->mode & (1 << (bit_offset + op))) {
		return 0;
	}
	return -1;
}


void ext2_dump_inode(struct inode* buf) {
	debug("%-19s: %d\n", "uid", buf->uid);
	debug("%-19s: %d\n", "gid", buf->gid);
	debug("%-19s: %d\n", "size", buf->size);
	debug("%-19s: %d\n", "block_count", buf->block_count);
	debug("%-19s: %d\n", "link_count", buf->link_count);
	debug("%-19s: %d\n", "atime", buf->atime);
	debug("%-19s: %d\n", "ctime", buf->ctime);
	debug("%-19s: %d\n", "mtime", buf->mtime);
	debug("%-19s: %d\n", "dtime", buf->dtime);

	debug("Blocks table:\n");
	for(uint32_t i = 0; i < 15; i++) {
		debug("\t%2d: 0x%x\n", i, buf->blocks[i]);
	}
}

#endif /* ENABLE_EXT2 */
