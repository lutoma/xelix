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

#include <fs/vfs.h>

enum inode_check_op {
	PERM_CHECK_READ = 2,
	PERM_CHECK_WRITE = 1,
	PERM_CHECK_EXEC = 0
};
int ext2_inode_check_perm(enum inode_check_op, struct inode* inode, task_t* task);

uint32_t ext2_bitmap_search_and_claim(struct ext2_fs* fs, uint32_t bitmap_block);
void ext2_bitmap_free(struct ext2_fs* fs, uint32_t bitmap_block, uint32_t bit);
