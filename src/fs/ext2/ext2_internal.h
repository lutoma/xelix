#pragma once

/* Copyright © 2013-2018 Lukas Martini
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

#include <hw/ide.h>
#include <lib/log.h>
#include <fs/vfs.h>
#include <fs/block.h>

#ifdef EXT2_DEBUG
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

struct inode {
	uint16_t mode;
	uint16_t uid;
	uint32_t size;
	uint32_t access_time;
	uint32_t creation_time;
	uint32_t modification_time;
	uint32_t deletion_time;
	uint16_t gid;
	uint16_t link_count;
	uint32_t block_count;
	uint32_t flags;
	uint32_t reserved1;
	uint32_t blocks[15];
	uint32_t version;
	uint32_t file_acl;
	uint32_t dir_acl;
	uint32_t fragment_address;
	uint8_t fragment_number;
	uint8_t fragment_size;
	uint16_t reserved2[5];
} __attribute__((packed));

#define SUPERBLOCK_MAGIC 0xEF53
#define SUPERBLOCK_STATE_CLEAN 1
#define SUPERBLOCK_STATE_DIRTY 2
#define ROOT_INODE 2

#define EXT2_INDEX_FL 0x00001000

#define inode_to_blockgroup(inode) ((inode - 1) / superblock->inodes_per_group)
// TODO Should use right shift for negative values
#define superblock_to_blocksize(superblock) (1024 << superblock->block_size)
#define bl_off(block) ((block) * superblock_to_blocksize(superblock))
#define write_superblock() vfs_block_write(1024, sizeof(struct superblock), (uint8_t*)superblock)

struct superblock* superblock;
struct blockgroup* blockgroup_table;
struct inode* root_inode;

bool ext2_write_inode(struct inode* buf, uint32_t inode_num);
bool ext2_read_inode(struct inode* buf, uint32_t inode_num);
uint32_t ext2_new_inode(struct inode** inodeptr);
uint32_t ext2_resolve_inode_blocknum(struct inode* inode, uint32_t block_num);
uint8_t* ext2_read_inode_blocks(struct inode* inode, uint32_t offset, uint32_t num, uint8_t* buf);
int ext2_write_inode_blocks(struct inode* inode, uint32_t inode_num, uint32_t num, uint8_t* buf);

uint32_t ext2_bitmap_search_and_claim(uint32_t bitmap_block);
uint32_t ext2_new_block();
size_t ext2_write_file(vfs_file_t* fp, void* source, size_t size);

size_t ext2_getdents(vfs_file_t* fp, void* dest, size_t size);
void ext2_insert_dirent(uint32_t dir, uint32_t inode, char* name, uint8_t type);

uint32_t ext2_open(char* path, uint32_t flags, void* mount_instance);
size_t ext2_read_file(vfs_file_t* fp, void* dest, size_t size);

static inline void dump_inode(struct inode* buf) {
	debug("%-19s: %d\n", "uid", buf->uid);
	debug("%-19s: %d\n", "gid", buf->gid);
	debug("%-19s: %d\n", "size", buf->size);
	debug("%-19s: %d\n", "block_count", buf->block_count);
	debug("%-19s: %d\n", "access_time", buf->access_time);
	debug("%-19s: %d\n", "creation_time", buf->creation_time);
	debug("%-19s: %d\n", "modification_time", buf->modification_time);

	debug("Blocks table:\n");
	for(uint32_t i = 0; i < 15; i++) {
		debug("\t%2d: 0x%x\n", i, buf->blocks[i]);
	}
}
