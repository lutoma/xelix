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

#define EXT2_DIRENT_FT_UNKNOWN 0
#define EXT2_DIRENT_FT_REG_FILE 1
#define EXT2_DIRENT_FT_DIR 2
#define EXT2_DIRENT_FT_CHRDEV 3
#define EXT2_DIRENT_FT_BLKDEV 4
#define EXT2_DIRENT_FT_FIFO 5
#define EXT2_DIRENT_FT_SOCk 6
#define EXT2_DIRENT_FT_SYMLINK 7

struct dirent {
	uint32_t inode;
	uint16_t record_len;
	uint8_t name_len;
	uint8_t type;
	char name[] __attribute__ ((nonstring));
} __attribute__((packed));

struct rd_r {
	uint8_t buf[0x200];
	size_t read_len;
	size_t read_off;
	size_t last_len;
};

struct dirent* ext2_dirent_find(struct ext2_fs* fs, const char* path, uint32_t* parent_ino, task_t* task);
void ext2_dirent_rm(struct ext2_fs* fs, uint32_t inode_num, char* name);
void ext2_dirent_add(struct ext2_fs* fs, uint32_t dir, uint32_t inode, char* name, uint8_t type);
vfs_dirent_t* ext2_readdir_r(struct ext2_fs* fs, struct inode* inode, uint64_t* offset, struct rd_r* reent);
