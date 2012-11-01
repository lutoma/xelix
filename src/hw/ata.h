#pragma once

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

/* controller and drive selection *******************************************/

struct ata_drive {
	uint8_t  flags;
	uint16_t signature;
	uint16_t capabilities;
	uint16_t commandsets;
	uint64_t size; /* size in sectors */
	char     model[41];
	uint16_t sectsize;

	uint8_t  irq;
	uint16_t base;
	uint16_t dma;

	bool mutex;
};

extern struct ata_drive ata_drives[4];

#define ATA0  0x00
#define ATA1  0x02

#define ATA00 0x00
#define ATA01 0x01
#define ATA10 0x02
#define ATA11 0x03

#define FLAG_EXIST  0x01
#define FLAG_DMA    0x02
#define FLAG_ATAPI  0x04
#define FLAG_LONG   0x08
#define FLAG_SATA   0x10
#define FLAG_LOCK   0x20

/* I/O port numbers *********************************************************/

#define REG_DATA    0x0000
#define REG_ERR     0x0001
#define REG_FEATURE 0x0001
#define REG_COUNT0  0x0002
#define REG_LBA0    0x0003
#define REG_LBA1    0x0004
#define REG_LBA2    0x0005
#define REG_SELECT  0x0006
#define REG_CMD     0x0007
#define REG_STAT    0x0007
#define REG_COUNT1  0x0008
#define REG_LBA3    0x0009
#define REG_LBA4    0x000A
#define REG_LBA5    0x000B
#define REG_CTRL    0x0206
#define REG_ASTAT   0x0206

/* command and status numbers ***********************************************/

#define CMD_READ_PIO      0x20
#define CMD_READ_PIO48    0x24
#define CMD_READ_DMA      0xC8
#define CMD_READ_DMA48    0x25
#define CMD_READ_ATAPI    0xA8
#define CMD_WRITE_PIO     0x30
#define CMD_WRITE_PIO48   0x34
#define CMD_WRITE_DMA     0xCA
#define CMD_WRITE_DMA48   0x35
#define CMD_CACHE_FLUSH   0xE7
#define CMD_CACHE_FLUSH48 0xEA
#define CMD_ID            0xEC
#define CMD_ATAPI_ID      0xA1
#define CMD_ATAPI_EJECT   0x1B
#define CMD_ATAPI         0xA0

#define STAT_ERROR   0x01
#define STAT_DRQ     0x08
#define STAT_SERVICE 0x10
#define STAT_FAULT   0x20
#define STAT_READY   0x40
#define STAT_BUSY    0x80

#define CTRL_NEIN    0x02
#define CTRL_RESET   0x04
#define CTRL_HBYTE   0x80

#define ID_TYPE      0x00
#define ID_SERIAL    0x0A
#define ID_MODEL     0x1B
#define ID_CAP       0x31
#define ID_VALID     0x35
#define ID_MAX_LBA   0x3C
#define ID_CMDSET    0x52
#define ID_MAX_LBA48 0x64

#define DMA_CMD_RUN    0x01
#define DMA_CMD_RW     0x08
#define DMA_STAT_READY 0x01
#define DMA_STAT_ERROR 0x02
#define DMA_STAT_IRQ   0x04
#define DMA_STAT_MDMA  0x20
#define DMA_STAT_SDMA  0x40

/* driver internal functions *************************************************/

#define SEL(d) ((d) & 1)
#define MASTER 0
#define SLAVE  1

void ata_select(uint8_t drive);

int ata_send_lba(uint8_t drive, uint64_t sector, uint16_t count);

/* PIO ***********************************************************************/

void pio_write(uint8_t drive, uint64_t sector, uint16_t count, uint16_t* buffer);
void ata_read (uint8_t drive, uint64_t sector, uint16_t count, uint16_t* buffer);

void ata_init(void);