#pragma once

/* Copyright © 2019 Lukas Martini
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
#include <kavl.h>
#include <fs/ext2/inode.h>

struct ftree_file {
	KAVL_HEAD(struct ftree_file) head;
	char path[512];

	vfs_stat_t stat;
	struct ftree_file* children;
};

struct ftree_file* vfs_ftree_insert(struct ftree_file* root, char* name, vfs_stat_t* stat);
struct ftree_file* vfs_ftree_insert_path(char* path, vfs_stat_t* stat);
const struct ftree_file* vfs_ftree_find(const struct ftree_file* root, char* name);
const struct ftree_file* vfs_ftree_find_path(char* path);
void vfs_ftree_init();
