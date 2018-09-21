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
#include <hw/ide.h>

uint8_t* vfs_block_read(uint32_t offset, size_t size, uint8_t* buf) {
	//serial_printf("vfs_block_read 0x%x size 0x%x to buffer 0x%x\n", offset, size, buf);

	int start_block = offset / 512;
	int num_blocks = size / 512;
	if(size % 512) {
		num_blocks++;
	}

	// Check if offset makes us cross another block boundary
	if(size % 512 + offset > 512) {
		num_blocks++;
	}

	uint8_t* int_buf = kmalloc(num_blocks * 512);
	for(int i = 0; i < num_blocks; i ++) {
		uint32_t bnum = start_block + i;

		//serial_printf("vfs/block: Reading block %d\n", bnum);
		if(!ide_read_sector(0x1F0, 0, bnum, int_buf + i * 512)) {
			kfree(int_buf);
			return NULL;
		}
	}

	memcpy(buf, int_buf + (offset % 512), size);
	kfree(int_buf);
	return buf;
}
