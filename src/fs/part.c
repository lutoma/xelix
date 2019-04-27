/* part.c: MBR partition support
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
#include <mem/kmalloc.h>
#include <hw/ide.h>

uint32_t start = 0;

struct mbr_partition {
	uint8_t bootable;
	uint8_t unused[3];
	uint8_t type;
	uint8_t unused2[3];
	uint32_t start;
	uint32_t length;
};

bool part_read(uint32_t lba, uint8_t * buf) {
	#ifdef __i386__
		return ide_read_sector(0x1F0, 0, lba + start, buf);
	#else
		return false;
	#endif
}
void part_write(uint32_t lba, uint8_t * buf) {
	#ifdef __i386__
		return ide_write_sector(0x1F0, 0, lba + start, buf);
	#else
		return;
	#endif
}

void part_init() {
	#ifdef __i386__
	uint8_t* buf = kmalloc(512);
	if(!ide_read_sector(0x1F0, 0, 0, buf)) {
		kfree(buf);
		return;
	}

	struct mbr_partition* part = (struct mbr_partition*)(buf + 0x01BE);
	start = part->start;
	#endif
}
