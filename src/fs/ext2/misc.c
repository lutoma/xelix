/* ext2.c: Implementation of the extended file system, version 2
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

#ifdef ENABLE_EXT2

#include "ext2_internal.h"
#include <memory/kmalloc.h>
#include <fs/ext2.h>
#include <fs/block.h>

uint32_t ext2_bitmap_search_and_claim(uint32_t bitmap_block) {
	// Todo check blockgroup->free_blocks to see if any blocks are free and otherwise switch block group
	uint8_t* bitmap = kmalloc(bl_off(1));
	vfs_block_read(bl_off(bitmap_block), bl_off(1), bitmap);
	uint32_t result = 0;

	for(int i = 0; i < bl_off(1); i++) {
		uint8_t* blt = bitmap + i;

		if(*blt != 0xff) {
			for(int j = 0; j < 8; j++) {
				if(!bit_get(*blt, j)) {
					*blt = bit_set(*blt, j);
					result = i*8 + j + 1;
					break;
				}
			}

			break;
		}
	}

	if(result) {
		vfs_block_write(bl_off(bitmap_block), bl_off(1), bitmap);
	}

	kfree(bitmap);
	return result;
}

void ext2_bitmap_free(uint32_t bitmap_block, uint32_t bit) {
	uint8_t* bitmap = kmalloc(bl_off(1));
	vfs_block_read(bl_off(bitmap_block), bl_off(1), bitmap);
	bit--;
	bitmap[bit / 8] = bit_clear(bitmap[bit / 8], bit % 8);
	vfs_block_write(bl_off(bitmap_block), bl_off(1), bitmap);
	kfree(bitmap);
}

char* ext2_chop_path(const char* path, char** ent) {
	char* base_path = strdup(path);
	char* c = base_path + strlen(path);
	for(; c > base_path; c--) {
		if(*c == '/') {
			*c = 0;

			if(ent) {
				*ent = c+1;
			}
			break;
		}
	}
	return base_path;
}

#endif /* ENABLE_EXT2 */
