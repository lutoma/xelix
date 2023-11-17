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

#include "ext2_internal.h"
#include <fs/vfs.h>

struct inode {
	uint16_t mode;
	uint16_t uid;
	uint32_t size;
	uint32_t atime;
	uint32_t ctime;
	uint32_t mtime;
	uint32_t dtime;
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

struct ext2_blocknum_resolver_cache {
	uint32_t* indirect_table;
	uint32_t* double_table;
	uint32_t double_second_block;
	uint32_t* double_second_table;
};

#define ext2_inode_read_data(fs, inode, offset, length, buf) ext2_inode_data_rw(fs, inode, 0, offset, length, buf)
#define ext2_inode_write_data ext2_inode_data_rw

struct ext2_fs;
bool ext2_inode_write(struct ext2_fs* fs, struct inode* buf, uint32_t inode_num);
bool ext2_inode_read(struct ext2_fs* fs, struct inode* buf, uint32_t inode_num);
uint32_t ext2_inode_new(struct ext2_fs* fs, struct inode* inode, uint16_t mode);
uint32_t ext2_resolve_blocknum(struct ext2_fs* fs, struct inode* inode, uint32_t block_num, struct ext2_blocknum_resolver_cache* cache);
void ext2_free_blocknum_resolver_cache(struct ext2_blocknum_resolver_cache* cache);
uint8_t* ext2_inode_data_rw(struct ext2_fs* fs, struct inode* inode, uint32_t write_inode_num,
	uint64_t offset, size_t length, uint8_t* buf);


uint32_t ext2_resolve_inode(const char* path, uint32_t* parent_ino);
void ext2_dump_inode(struct inode* buf);
