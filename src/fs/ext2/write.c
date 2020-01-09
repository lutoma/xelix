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
#include <log.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>

uint32_t ext2_block_new(struct ext2_fs* fs, uint32_t neighbor) {
	uint32_t pref_blockgroup = inode_to_blockgroup(neighbor);

	struct blockgroup* blockgroup = fs->blockgroup_table + pref_blockgroup;
	if(!blockgroup || !blockgroup->inode_table) {
		log(LOG_ERR, "ext2: Could not locate entry %d in blockgroup table\n", pref_blockgroup);
		return 0;
	}

	uint32_t block_num = ext2_bitmap_search_and_claim(fs, blockgroup->block_bitmap);
	if(!block_num) {
		log(LOG_ERR, "ext2: Could not find free block in preferred blockgroup %d.\n", pref_blockgroup);
		return 0;
	}

	fs->superblock->free_blocks--;
	blockgroup->free_blocks--;
	write_superblock();
	write_blockgroup_table();
	return block_num;

}
#endif /* ENABLE_EXT2 */
