/* ext2_misc.c: Miscellaneous ext2 helpers
 * Copyright © 2018-2019 Lukas Martini
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

#ifdef CONFIG_ENABLE_EXT2

#include "ext2_internal.h"
#include "ext2_misc.h"
#include <mem/kmalloc.h>
#include <block/block.h>
#include <bitmap.h>

uint32_t ext2_bitmap_search_and_claim(struct ext2_fs* fs, uint32_t bitmap_block) {
	// Todo check blockgroup->free_blocks to see if any blocks are free and otherwise switch block group
	uint8_t* bitmap = kmalloc(bl_off(1));
	vfs_block_sread(fs->dev, bl_off(bitmap_block), bl_off(1), bitmap);
	uint32_t result = 0;

	for(int i = 0; i < bl_off(1); i++) {
		uint8_t* blt = bitmap + i;

		if(*blt != 0xff) {
			for(int j = 0; j < 8; j++) {
				if(!bit_get(*blt, j)) {
					*blt = bit_set(*blt, j);
					result = i*8 + j;
					break;
				}
			}

			break;
		}
	}

	if(result) {
		vfs_block_swrite(fs->dev, bl_off(bitmap_block), bl_off(1), bitmap);
	}

	kfree(bitmap);
	return result;
}

void ext2_bitmap_free(struct ext2_fs* fs, uint32_t bitmap_block, uint32_t bit) {
	uint8_t* bitmap = kmalloc(bl_off(1));
	vfs_block_sread(fs->dev, bl_off(bitmap_block), bl_off(1), bitmap);
	bitmap[bit / 8] = bit_clear(bitmap[bit / 8], bit % 8);
	vfs_block_swrite(fs->dev, bl_off(bitmap_block), bl_off(1), bitmap);
	kfree(bitmap);
}

const char* ext2_chop_path(const char* path, const char** ent) {
	char* parent_dir = strdup(path);
	char* file_name = strrchr(parent_dir, '/');
	if(!file_name || parent_dir == file_name) {
		if(ent) {
			*ent = parent_dir;
		}
		return "/";
	} else {
		*file_name = '\0';
		if(ent) {
			*ent = file_name++;
		}
		return parent_dir;
	}
}

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

#endif /* CONFIG_ENABLE_EXT2 */
