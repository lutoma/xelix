/*
 * Copyright (C) 2011 Nick Johnson <nickbjohnson4224 at gmail.com>
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <lib/generic.h>
#include <lib/stdint.h>
#include <lib/string.h>
#include <lib/log.h>
#include <lib/print.h>

#include "ata.h"

struct ata_drive ata_drives[4];

void ata_sleep400(uint8_t drive)
{

	inb(ata_drives[drive].base + REG_ASTAT);
	inb(ata_drives[drive].base + REG_ASTAT);
	inb(ata_drives[drive].base + REG_ASTAT);
	inb(ata_drives[drive].base + REG_ASTAT);
}

int ata_wait(uint8_t drive)
{
	uint8_t status;
	
	for(int i = 0;; i++) {
		status = inb(ata_drives[drive].base + REG_STAT);

		if (!(status & STAT_BUSY)) {
			return 0;
		}

		if (i > 4 && ((status & STAT_ERROR) || (status & STAT_FAULT))) {
			return status;
		}
	}
}

void ata_select(uint8_t drive)
{

	outb(ata_drives[drive].base + REG_SELECT, SEL(drive) ? 0xB0 : 0xA0);
	ata_sleep400(drive);
}

int ata_send_lba(uint8_t drive, uint64_t sector, uint16_t count)
{
	uint8_t lba[8];
	uint8_t head;
	int lba48;

	if (count > 256)
		return -1;
	else if (count == 256) 
		count = 0;

	ata_select(drive);

	/* format LBA bytes from sector index */
	/* use LBA28 */
	lba48  = 0;
	head   = (sector >> 24) & 0xF;
	lba[2] = (sector >> 16) & 0xFF;
	lba[1] = (sector >> 8)  & 0xFF;
	lba[0] = (sector >> 0)  & 0xFF;

	/* wait for drive to be ready */
	if (ata_wait(drive)) return -1;

	/* send LBA */
	outb(ata_drives[drive].base + REG_SELECT, (SEL(drive) ? 0xF0 : 0xE0) | head);
	ata_sleep400(drive);
	outb(ata_drives[drive].base + REG_COUNT0, count);
	outb(ata_drives[drive].base + REG_LBA0, lba[0]);
	outb(ata_drives[drive].base + REG_LBA1, lba[1]);
	outb(ata_drives[drive].base + REG_LBA2, lba[2]);
	
	return lba48;
}

void ata_read(uint8_t drive, uint64_t sector, uint16_t count, uint16_t *buffer)
{
	size_t i, j;
	uint16_t word;
	int lba48 = 0;

	lba48 = ata_send_lba(drive, sector, count);
	if (lba48 == -1)
	{
		log(LOG_ERR, "ata: ata_pio_read: ata_send_lba failed.\n");
		return;
	}

	outb(ata_drives[drive].base + REG_CMD, CMD_READ_PIO);

	if (ata_wait(drive))
	{
		log(LOG_ERR, "ata: ata_pio_read: ata_wait timed out.\n");
		return;
	}

	for (j = 0; j < count; j++) {
		for (i = 0; i < (uint32_t) (1 << ata_drives[drive].sectsize) >> 1; i++) {
			word = inw(ata_drives[drive].base + REG_DATA);
			buffer[i + j * (1 << (ata_drives[drive].sectsize - 1))] = word;
		}
		ata_sleep400(drive);
	}
}

void ata_init(void)
{
	log(LOG_INFO, "ata: Setting up\n");
	size_t dr, i;
	uint8_t status, err, cl, ch;
	uint16_t c;
	uint16_t buffer[256];

	/* detect I/O ports */
	ata_drives[ATA0].base = 0x1F0;
	ata_drives[ATA1].base = 0x170;
	ata_drives[ATA0].dma  = 0; // FIXME
	ata_drives[ATA1].dma  = 0; // FIXME

	/* detect IRQ numbers */
	ata_drives[ATA0].irq = 14;
	ata_drives[ATA1].irq = 15;

	/* copy controller ports for drives */
	/* ATA 0 master */
	ata_drives[ATA00].base = ata_drives[ATA0].base;
	ata_drives[ATA00].dma  = ata_drives[ATA0].dma;
	ata_drives[ATA00].irq  = ata_drives[ATA0].irq;

	/* ATA 0 slave */
	ata_drives[ATA01].base = ata_drives[ATA0].base;
	ata_drives[ATA01].dma  = ata_drives[ATA0].dma;
	ata_drives[ATA01].irq  = ata_drives[ATA0].irq;

	/* ATA 1 master */
	ata_drives[ATA10].base = ata_drives[ATA1].base;
	ata_drives[ATA10].dma  = ata_drives[ATA1].dma;
	ata_drives[ATA10].irq  = ata_drives[ATA1].irq;

	/* ATA 1 slave */
	ata_drives[ATA11].base = ata_drives[ATA1].base;
	ata_drives[ATA11].dma  = ata_drives[ATA1].dma;
	ata_drives[ATA11].irq  = ata_drives[ATA1].irq;

	/* disable controller IRQs */
	outw(ata_drives[ATA0].base + REG_CTRL, CTRL_NEIN);
	outw(ata_drives[ATA1].base + REG_CTRL, CTRL_NEIN);


	log(LOG_INFO, "ata: Searching devices\n");
	/* detect drives */
	for (dr = 0; dr < 4; dr++) {
		ata_select(dr);

		/* send IDENTIFY command */
		outb(ata_drives[dr].base + REG_COUNT0, 0);
		outb(ata_drives[dr].base + REG_LBA0,   0);
		outb(ata_drives[dr].base + REG_LBA1,   0);
		outb(ata_drives[dr].base + REG_LBA2,   0);
		ata_sleep400(dr);
		outb(ata_drives[dr].base + REG_CMD, CMD_ID);
		ata_sleep400(dr);

		/* read status */
		status = inb(ata_drives[dr].base + REG_STAT);

		/* check for drive response */
		if (!status) continue;

		ata_drives[dr].flags = FLAG_EXIST;

		/* poll for response */
		err = ata_wait(dr);

		/* try to catch ATAPI and SATA devices */
		if (err && (inb(ata_drives[dr].base + REG_LBA1) || inb(ata_drives[dr].base + REG_LBA2))) {
			cl = inb(ata_drives[dr].base + REG_LBA1);
			ch = inb(ata_drives[dr].base + REG_LBA2);
			c = cl | (ch << 8);
			
			ata_sleep400(dr);

			if (c == 0xEB14 || c == 0x9669) {
				/* is ATAPI */
				ata_drives[dr].flags |= FLAG_ATAPI;
				ata_drives[dr].sectsize = 11;

				/* use ATAPI IDENTIFY */
				outb(ata_drives[dr].base + REG_CMD, CMD_ATAPI_ID);
				ata_sleep400(dr);
			}
			else if (c == 0xC33C) {
				/* is SATA */
				ata_drives[dr].flags |= FLAG_SATA;
			}
			else {
				/* unknown device; ignore */
				ata_drives[dr].flags = 0;
				continue;
			}
		}
		else {
			/* assume 512-byte sectors for ATA */
			ata_drives[dr].sectsize = 9;
		}

		/* wait for IDENTIFY to be ready */
		err = ata_wait(dr);

		/* read in IDENTIFY space */
		for (i = 0; i < 256; i++) {
			buffer[i] = inw(ata_drives[dr].base + REG_DATA);
		}

		if (!buffer[ID_TYPE]) {
			/* no drive */
			ata_drives[dr].flags = 0;
			continue;
		}

		ata_drives[dr].signature    = buffer[ID_TYPE];
		ata_drives[dr].capabilities = buffer[ID_CAP];
		ata_drives[dr].commandsets  = buffer[ID_CMDSET] | (uint32_t) buffer[ID_CMDSET+1] << 16;


		/* is LBA24 */
		ata_drives[dr].size  = (uint32_t) buffer[ID_MAX_LBA+0];
		ata_drives[dr].size |= (uint32_t) buffer[ID_MAX_LBA+1] << 16;

		/* get model string */
		for (i = 0; i < 40; i += 2) {
			ata_drives[dr].model[i]   = buffer[ID_MODEL + (i / 2)] >> 8;
			ata_drives[dr].model[i+1] = buffer[ID_MODEL + (i / 2)] & 0xFF;
		}
		ata_drives[dr].model[40] = '\0';
		for (i = 39; i > 0; i--) {
			if (ata_drives[dr].model[i] == ' ') {
				ata_drives[dr].model[i] = '\0';
			}
			else break;
		}

		log(LOG_INFO, "ata: Found drive: ");

		switch (dr) {
		case ATA00: print("ATA 0 Master\n"); break;
		case ATA01: print("ATA 0 Slave\n");  break;
		case ATA10: print("ATA 1 Master\n"); break;
		case ATA11: print("ATA 1 Slave\n");  break;
		}

		print("\t");
		print((ata_drives[dr].flags & FLAG_SATA) ? "S" : "P");
		print((ata_drives[dr].flags & FLAG_ATAPI) ? "ATAPI " : "ATA ");
		print((ata_drives[dr].flags & FLAG_LONG) ? "LBA48 " : "LBA24 ");
		print("\n");

		printf("\tsize: %d KB (%d sectors)\n",
			(uint32_t) ata_drives[dr].size * (1 << ata_drives[dr].sectsize) >> 10,
			(uint32_t) ata_drives[dr].size);
		printf("\tmodel: %s\n", ata_drives[dr].model);


	}
}
