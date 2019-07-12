/* part.c: MBR partition support
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

#include <string.h>
#include <mem/kmalloc.h>
#include <fs/i386-ide.h>
#include <fs/block.h>

struct mbr_partition {
	uint8_t bootable;
	uint8_t unused[3];
	uint8_t type;
	uint8_t unused2[3];
	uint32_t start;
	uint32_t size;
};

void vfs_part_probe(struct vfs_block_dev* dev) {
	uint8_t* buf = kmalloc(512);
	if(dev->read_cb(dev, 0, buf) < 0) {
		kfree(buf);
		return;
	}

	char* pname = kmalloc(strlen(dev->name) + 3);
	struct mbr_partition* part = (struct mbr_partition*)(buf + 0x01BE);
	for(int i = 1; i <= 4; i++, part++) {
		if(!part->type || !part->size || !part->start) {
			continue;
		}

		sprintf(pname, "%sp%d", dev->name, i);
		vfs_block_register_dev(pname, part->start, dev->read_cb, dev->write_cb, dev->meta);
		log(LOG_INFO, "part: /dev/%s: MBR part %d /dev/%s type %x size %#x\n",
			dev->name, i, pname, part->type, part->size);
	}

	kfree(pname);
	kfree(buf);
}
