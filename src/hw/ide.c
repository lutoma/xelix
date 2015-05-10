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

#include <lib/generic.h>
#include <lib/log.h>
#include <memory/kmalloc.h>
#include <interrupts/interface.h>
#include "ide.h"

ide_channel_regs_t ide_channels[2];
ide_device_t ide_devices[4];
uint8_t ide_buf[2048] = {0};
uint8_t ide_irq_invoked = 0;
uint8_t atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void inportsm(unsigned short port, unsigned char * data, unsigned long size) {
	asm volatile ("rep insw" : "+D" (data), "+c" (size) : "d" (port) : "memory");
}

void outportsm(unsigned short port, unsigned char * data, unsigned long size) {
	asm volatile ("rep outsw" : "+S" (data), "+c" (size) : "d" (port));
}

void ata_io_wait(uint16_t bus) {
	inb(bus + ATA_REG_ALTSTATUS);
	inb(bus + ATA_REG_ALTSTATUS);
	inb(bus + ATA_REG_ALTSTATUS);
	inb(bus + ATA_REG_ALTSTATUS);
}

int ata_wait(uint16_t bus, int advanced) {
	uint8_t status = 0;

	ata_io_wait(bus);

	while ((status = inb(bus + ATA_REG_STATUS)) & ATA_SR_BSY);

	if (advanced) {
		status = inb(bus + ATA_REG_STATUS);
		if (status   & ATA_SR_ERR)  return 1;
		if (status   & ATA_SR_DF)   return 1;
		if (!(status & ATA_SR_DRQ)) return 1;
	}

	return 0;
}

void ata_select(uint16_t bus) {
	outb(bus + ATA_REG_HDDEVSEL, 0xA0);
}

void ata_wait_ready(uint16_t bus) {
	while (inb(bus + ATA_REG_STATUS) & ATA_SR_BSY);
}

void ide_init_device(uint16_t bus) {

	log(LOG_INFO, "ide: Initializing IDE device on bus %d\n", bus);

	outb(bus + 1, 1);
	outb(bus + 0x306, 0);

	ata_select(bus);
	ata_io_wait(bus);

	outb(bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	ata_io_wait(bus);
	ata_wait_ready(bus);

	ata_identify_t device;
	uint16_t * buf = (uint16_t *)&device;

	for (int i = 0; i < 256; ++i) {
		buf[i] = portio_in16(bus);
	}

	uint8_t * ptr = (uint8_t *)&device.model;
	for (int i = 0; i < 39; i+=2) {
		uint8_t tmp = ptr[i+1];
		ptr[i+1] = ptr[i];
		ptr[i] = tmp;
	}

	outb(bus + ATA_REG_CONTROL, 0x02);
}

void ide_init() {
	ide_init_device(0x1F0);
}

bool ide_read_sector(uint16_t bus, uint8_t slave, uint32_t lba, uint8_t * buf) {
	int errors = 0;
try_again:
	outb(bus + ATA_REG_CONTROL, 0x02);

	ata_wait_ready(bus);

	outb(bus + ATA_REG_HDDEVSEL,  0xe0 | slave << 4 | 
								 (lba & 0x0f000000) >> 24);
	outb(bus + ATA_REG_FEATURES, 0x00);
	outb(bus + ATA_REG_SECCOUNT0, 1);
	outb(bus + ATA_REG_LBA0, (lba & 0x000000ff) >>  0);
	outb(bus + ATA_REG_LBA1, (lba & 0x0000ff00) >>  8);
	outb(bus + ATA_REG_LBA2, (lba & 0x00ff0000) >> 16);
	outb(bus + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

	if (ata_wait(bus, 1)) {
		errors++;
		if (errors > 4) {
			log(LOG_WARN, "ide: Too many errors during read of lba block %d. Bailing.\n");
			return false;
		}
		goto try_again;
	}

	int size = 256;
	inportsm(bus,buf,size);
	ata_wait(bus, 0);
	return true;
}

void ide_write_sector(uint16_t bus, uint8_t slave, uint32_t lba, uint8_t * buf) {
	outb(bus + ATA_REG_CONTROL, 0x02);

	ata_wait_ready(bus);

	outb(bus + ATA_REG_HDDEVSEL,  0xe0 | slave << 4 | 
								 (lba & 0x0f000000) >> 24);
	ata_wait(bus, 0);
	outb(bus + ATA_REG_FEATURES, 0x00);
	outb(bus + ATA_REG_SECCOUNT0, 0x01);
	outb(bus + ATA_REG_LBA0, (lba & 0x000000ff) >>  0);
	outb(bus + ATA_REG_LBA1, (lba & 0x0000ff00) >>  8);
	outb(bus + ATA_REG_LBA2, (lba & 0x00ff0000) >> 16);
	outb(bus + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
	ata_wait(bus, 0);
	int size = 256;
	outportsm(bus,buf,size);
	outb(bus + 0x07, ATA_CMD_CACHE_FLUSH);
	ata_wait(bus, 0);
}

int ide_cmp(uint32_t * ptr1, uint32_t * ptr2, size_t size) {
	if(!(size % 4))
		return -1;

	uint32_t i = 0;
	while (i < size) {
		if (*ptr1 != *ptr2) return 1;
		ptr1++;
		ptr2++;
		i += 4;
	}
	return 0;
}

void ide_write_sector_retry(uint16_t bus, uint8_t slave, uint32_t lba, uint8_t * buf) {
	uint8_t * read_buf = kmalloc(512);
	interrupts_disable();
	do {
		ide_write_sector(bus,slave,lba,buf);
		ide_read_sector(bus,slave,lba,read_buf);
	} while (ide_cmp((uint32_t*)buf,(uint32_t*)read_buf,512));
	kfree(read_buf);
	interrupts_enable();
}