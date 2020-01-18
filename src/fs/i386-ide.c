/* ide.c: IDE driver
 * Copyright © 2011-2013 Kevin Lange
 * Copyright © 2013 Lukas Martini
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
 *
 * This file was originally released under the NCSA license as part of ToAruOS.
 * See https://github.com/klange/osdev/blob/strawberry-dev/kernel/devices/ide.c
 */

#include <log.h>
#include <mem/kmalloc.h>
#include <int/int.h>
#include <portio.h>
#include <fs/i386-ide.h>
#include <fs/block.h>

struct ata_identify {
	uint16_t flags;
	uint16_t unused1[9];
	char     serial[20];
	uint16_t unused2[3];
	char     firmware[8];
	char     model[40];
	uint16_t sectors_per_int;
	uint16_t unused3;
	uint16_t capabilities[2];
	uint16_t unused4[2];
	uint16_t valid_ext_data;
	uint16_t unused5[5];
	uint16_t size_of_rw_mult;
	uint32_t sectors_28;
	uint16_t unused6[38];
	uint64_t sectors_48;
	uint16_t unused7[152];
};

struct ide_dev {
	uint16_t bus;
	uint8_t slave;
};

void inportsm(unsigned short port, unsigned char * data, unsigned long size) {
	asm volatile ("rep insw" : "+D" (data), "+c" (size) : "d" (port) : "memory");
}

void outportsm(unsigned short port, unsigned char * data, unsigned long size) {
	asm volatile ("rep outsw" : "+S" (data), "+c" (size) : "d" (port));
}

void ata_io_wait(struct ide_dev* dev) {
	inb(dev->bus + ATA_REG_ALTSTATUS);
	inb(dev->bus + ATA_REG_ALTSTATUS);
	inb(dev->bus + ATA_REG_ALTSTATUS);
	inb(dev->bus + ATA_REG_ALTSTATUS);
}

int ata_wait(struct ide_dev* dev, int advanced) {
	uint8_t status = 0;

	ata_io_wait(dev);

	while ((status = inb(dev->bus + ATA_REG_STATUS)) & ATA_SR_BSY);

	if (advanced) {
		status = inb(dev->bus + ATA_REG_STATUS);
		if (status   & ATA_SR_ERR)  return 1;
		if (status   & ATA_SR_DF)   return 1;
		if (!(status & ATA_SR_DRQ)) return 1;
	}

	return 0;
}

void ata_select(struct ide_dev* dev) {
	outb(dev->bus + ATA_REG_HDDEVSEL, 0xA0);
}

void ata_wait_ready(struct ide_dev* dev) {
	while (inb(dev->bus + ATA_REG_STATUS) & ATA_SR_BSY);
}

struct ide_dev* ide_init_device(uint16_t bus) {
	struct ide_dev* dev = zmalloc(sizeof(struct ide_dev));
	dev->bus = bus;
	dev->slave = 0;

	log(LOG_INFO, "ide: Initializing IDE device on bus %#x\n", dev->bus);
	outb(dev->bus + 1, 1);
	outb(dev->bus + 0x306, 0);

	ata_select(dev);
	ata_io_wait(dev);

	outb(dev->bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	ata_io_wait(dev);
	ata_wait_ready(dev);

	struct ata_identify device;
	uint16_t * buf = (uint16_t *)&device;

	for (int i = 0; i < 256; ++i) {
		buf[i] = inw(dev->bus);
	}

	uint8_t * ptr = (uint8_t *)&device.model;
	for (int i = 0; i < 39; i+=2) {
		uint8_t tmp = ptr[i+1];
		ptr[i+1] = ptr[i];
		ptr[i] = tmp;
	}

	outb(dev->bus + ATA_REG_CONTROL, 0x02);
	return dev;
}
static inline int do_read(struct ide_dev* dev, uint64_t lba, void* buf) {
	int errors = 0;
try_again:
	outb(dev->bus + ATA_REG_CONTROL, 0x02);

	ata_wait_ready(dev);

	outb(dev->bus + ATA_REG_HDDEVSEL,  0xe0 | dev->slave << 4 |
								 (lba & 0x0f000000) >> 24);
	outb(dev->bus + ATA_REG_FEATURES, 0x00);
	outb(dev->bus + ATA_REG_SECCOUNT0, 1);
	outb(dev->bus + ATA_REG_LBA0, (lba & 0x000000ff) >>  0);
	outb(dev->bus + ATA_REG_LBA1, (lba & 0x0000ff00) >>  8);
	outb(dev->bus + ATA_REG_LBA2, (lba & 0x00ff0000) >> 16);
	outb(dev->bus + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

	if (ata_wait(dev, 1)) {
		errors++;
		if (errors > 4) {
			log(LOG_WARN, "ide: Too many errors during read of lba block %u. Bailing.\n", lba);
			return -1;
		}
		goto try_again;
	}

	int size = 256;
	inportsm(dev->bus, buf, size);
	ata_wait(dev, 0);
	return 0;

}

uint64_t ide_read_cb(struct vfs_block_dev* block_dev, uint64_t lba, uint64_t num_blocks, void* buf) {
	struct ide_dev* dev = (struct ide_dev*)block_dev->meta;

	for(int i = 0; i < num_blocks; i++) {
		if(do_read(dev, lba + i, buf + i * 512) < 0) {
			return i;
		}
	}

	return num_blocks;
}

static inline int do_write(struct ide_dev* dev, uint64_t lba, void* buf) {
	outb(dev->bus + ATA_REG_CONTROL, 0x02);

	ata_wait_ready(dev);

	outb(dev->bus + ATA_REG_HDDEVSEL,  0xe0 | dev->slave << 4 |
								 (lba & 0x0f000000) >> 24);
	ata_wait(dev, 0);
	outb(dev->bus + ATA_REG_FEATURES, 0x00);
	outb(dev->bus + ATA_REG_SECCOUNT0, 0x01);
	outb(dev->bus + ATA_REG_LBA0, (lba & 0x000000ff) >>  0);
	outb(dev->bus + ATA_REG_LBA1, (lba & 0x0000ff00) >>  8);
	outb(dev->bus + ATA_REG_LBA2, (lba & 0x00ff0000) >> 16);
	outb(dev->bus + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
	ata_wait(dev, 0);
	int size = 256;
	outportsm(dev->bus, buf, size);
	outb(dev->bus + 0x07, ATA_CMD_CACHE_FLUSH);
	ata_wait(dev, 0);
	return 0;
}

uint64_t ide_write_cb(struct vfs_block_dev* block_dev, uint64_t lba, uint64_t num_blocks, void* buf) {
	struct ide_dev* dev = (struct ide_dev*)block_dev->meta;

	for(int i = 0; i < num_blocks; i++) {
		if(do_write(dev, lba + i, buf + i * 512) < 0) {
			return i;
		}
	}

	return num_blocks;
}

void ide_init() {
	struct ide_dev* dev = ide_init_device(0x1F0);
	vfs_block_register_dev("ide1", 0, ide_read_cb, ide_write_cb, (void*)dev);
}
