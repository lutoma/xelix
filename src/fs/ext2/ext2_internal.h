#pragma once

/* Copyright © 2013-2019 Lukas Martini
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

struct dirent {
	uint32_t inode;
	uint16_t record_len;
	uint8_t name_len;
	uint8_t type;
	char name[] __attribute__ ((nonstring));
} __attribute__((packed));

#define SUPERBLOCK_MAGIC 0xEF53
#define SUPERBLOCK_STATE_CLEAN 1
#define SUPERBLOCK_STATE_DIRTY 2
#define ROOT_INODE 2

#define EXT2_INDEX_FL 0x00001000

#define inode_to_blockgroup(inode) ((inode - 1) / superblock->inodes_per_group)
// TODO Should use right shift for negative values (XXX: negative ???)
#define superblock_to_blocksize(superblock) (1024 << superblock->block_size)
#define bl_off(block) ((block) * superblock_to_blocksize(superblock))
#define bl_size(block) ((block) / superblock_to_blocksize(superblock))
#define bl_mod(block) ((block) % superblock_to_blocksize(superblock))

/* The number of blocks occupied by the blockgroup table
 * There doesn't seem to be a way to directly get the number of blockgroups,
 * so figure it out by dividing block count with blocks per group. Multiply
 * with struct size to get total space required, then divide by block size
 * to get ext2 blocks. Add 1 since partially used blocks also need to be
 * allocated.
 */
#define blockgroup_table_size (bl_size(superblock->block_count / superblock->blocks_per_group * sizeof(struct blockgroup)) + 1)

#define write_superblock() vfs_block_write(1024, sizeof(struct superblock), (uint8_t*)superblock)
#define write_blockgroup_table() vfs_block_write(bl_off(2), bl_off(blockgroup_table_size), (uint8_t*)blockgroup_table)

struct superblock* superblock;
struct blockgroup* blockgroup_table;
struct inode* root_inode;

bool ext2_inode_write(struct inode* buf, uint32_t inode_num);
bool ext2_inode_read(struct inode* buf, uint32_t inode_num);
uint32_t ext2_inode_new(struct inode* inode, uint16_t mode);
uint8_t* ext2_inode_read_data(struct inode* inode, uint32_t offset, size_t length, uint8_t* buf);
uint8_t* ext2_inode_write_data(struct inode* inode, uint32_t inode_num, uint32_t offset, size_t length, uint8_t* buf);

uint32_t ext2_bitmap_search_and_claim(uint32_t bitmap_block);
char* ext2_chop_path(const char* path, char** ent);

uint32_t ext2_block_new();

size_t ext2_write(vfs_file_t* fp, void* source, size_t size);
size_t ext2_getdents(vfs_file_t* fp, void* dest, size_t size);
struct dirent* ext2_dirent_find(struct inode* inode, const char* search);
void ext2_dirent_rm(uint32_t inode_num, char* name);
void ext2_dirent_add(uint32_t dir, uint32_t inode, char* name, uint8_t type);

uint32_t ext2_resolve_inode(const char* path, uint32_t* parent_ino);
uint32_t ext2_open(char* path, uint32_t flags, void* mount_instance);

size_t ext2_do_read(vfs_file_t* fp, void* dest, size_t size, uint32_t req_type);
size_t ext2_read(vfs_file_t* fp, void* dest, size_t size);

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
