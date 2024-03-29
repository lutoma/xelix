#pragma once

/* Copyright © 2013-2020 Lukas Martini
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

#include "ext2.h"
#include "ext2_inode.h"
#include <lib/log.h>
#include <fs/vfs.h>
#include <block/block.h>
#include <tasks/task.h>

#ifdef CONFIG_EXT2_DEBUG
  #define debug(args...) log(LOG_DEBUG, "ext2: " args)
#else
  #define debug(...)
#endif

struct superblock {
	uint32_t inode_count;
	uint32_t block_count;
	uint32_t reserved_blocks;
	uint32_t free_blocks;
	uint32_t free_inodes;
	uint32_t first_data_block;
	uint32_t block_size;
	int32_t fragment_size;
	uint32_t blocks_per_group;
	uint32_t fragments_per_group;
	uint32_t inodes_per_group;
	uint32_t mount_time;
	uint32_t write_time;
	uint16_t mount_count;
	int16_t max_mount_count;
	uint16_t magic;
	uint16_t state;
	uint16_t errors;
	uint16_t minor_revision;
	uint32_t last_check_time;
	uint32_t check_interval;
	uint32_t creator_os;
	uint32_t revision;
	uint16_t default_res_uid;
	uint16_t default_res_gid;
	uint32_t first_inode;
	uint16_t inode_size;
	uint16_t blockgroup_num;
	uint32_t features_compat;
	uint32_t features_incompat;
	uint32_t features_ro;
	uint32_t volume_id[4];
	char volume_name[16];
	char last_mounted[64];
	uint32_t algo_bitmap;
	uint32_t reserved[205];
} __attribute__((packed));

struct blockgroup {
	uint32_t block_bitmap;
	uint32_t inode_bitmap;
	uint32_t inode_table;
	uint16_t free_blocks;
	uint16_t free_inodes;
	uint16_t used_directories;
	uint16_t padding;
	uint32_t reserved[3];
} __attribute__((packed));

#define INODE_CACHE_MAX 0x100
struct inode_cache_entry {
	uint32_t num;
	struct inode inode;
};

struct ext2_fs {
	struct vfs_block_dev* dev;
	struct superblock* superblock;
	struct blockgroup* blockgroup_table;
	struct inode* root_inode;
	struct vfs_callbacks* callbacks;

	struct inode_cache_entry inode_cache[INODE_CACHE_MAX];
	uint32_t inode_cache_end;
};

#define SUPERBLOCK_MAGIC 0xEF53
#define SUPERBLOCK_STATE_CLEAN 1
#define SUPERBLOCK_STATE_DIRTY 2
#define ROOT_INODE 2

#define EXT2_INDEX_FL 0x00001000

#define inode_to_blockgroup(inode) ((inode - 1) / fs->superblock->inodes_per_group)

#define _block_size(fs) (1024 << fs->superblock->block_size)
#define bl_off(block) (uint64_t)((uint64_t)(block) * _block_size(fs))
#define bl_size(block) (uint64_t)((uint64_t)(block) / _block_size(fs))
#define bl_mod(block) (uint64_t)((uint64_t)(block) % _block_size(fs))

/* Blockgroup table is located in the block following the superblock. This
 * is usually the second block, but with a 1k block size, the superblock
 * takes up two blocks and the blockgroup table thus starts in block 3.
 */
#define blockgroup_table_start (bl_off(1) == 1024 ? 2 : 1)

/* The number of blocks occupied by the blockgroup table
 * There doesn't seem to be a way to directly get the number of blockgroups,
 * so figure it out by dividing block count with blocks per group. Multiply
 * with struct size to get total space required, then divide by block size
 * to get ext2 blocks. Add 1 since partially used blocks also need to be
 * allocated.
 */
#define blockgroup_table_size (bl_size(fs->superblock->block_count / fs->superblock->blocks_per_group * sizeof(struct blockgroup)) + 1)

#define write_superblock() vfs_block_swrite(fs->dev, 1024, sizeof(struct superblock), (uint8_t*)fs->superblock)
#define write_blockgroup_table() vfs_block_swrite(fs->dev, bl_off(blockgroup_table_start), \
	bl_off(blockgroup_table_size), (uint8_t*)fs->blockgroup_table)

uint32_t ext2_block_new(struct ext2_fs* fs, uint32_t neighbor);
