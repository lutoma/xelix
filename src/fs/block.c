/* block.c: Translate block-based devices to streams
 * Copyright © 2018 Lukas Martini
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

#include <string.h>
#include <memory/kmalloc.h>
#include <fs/part.h>

static uint8_t* do_read(int start_block, int num_blocks) {
	uint8_t* buf = kmalloc(num_blocks * 512);
	for(int i = 0; i < num_blocks; i++) {
		uint32_t bnum = start_block + i;

		//serial_printf("vfs/block: Reading block %d\n", bnum);
		if(!part_read(bnum, buf + i * 512)) {
			kfree(buf);
			return NULL;
		}
	}

	return buf;
}

static inline int calc_nb(uint32_t offset, uint32_t size) {
	int num_blocks = size / 512;

	if(size % 512) {
		num_blocks++;
	}

	// Check if offset makes us cross another block boundary
	if(size % 512 + offset > 512) { // FIXME Shouldn't this be offset % 512
		num_blocks++;
	}

	return num_blocks;
}

uint8_t* vfs_block_read(uint32_t offset, size_t size, uint8_t* buf) {
	int start_block = offset / 512;
	int num_blocks = calc_nb(offset, size);

	uint8_t* int_buf = do_read(start_block, num_blocks);
	if(!int_buf) {
		return NULL;
	}

	memcpy(buf, int_buf + (offset % 512), size);
	kfree(int_buf);
	return buf;
}

bool vfs_block_write(uint32_t offset, size_t size, uint8_t* buf) {
	int start_block = offset / 512;
	int num_blocks = calc_nb(offset, size);

	uint8_t* int_buf = do_read(start_block, num_blocks);
	if(!int_buf) {
		return false;
	}

	memcpy(int_buf + (offset % 512), buf, size);
	for(int i = 0; i < num_blocks; i++) {
		uint32_t bnum = start_block + i;
		//serial_printf("vfs/block: Writing block %d\n", bnum);
		part_write(bnum, int_buf + i * 512);
	}

	kfree(int_buf);
	return true;
}
