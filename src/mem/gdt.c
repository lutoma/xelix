/* gdt.c: Disable segmentation by defining one large segment
 * Copyright © 2010 Christoph Sünderhauf
 * Copyright © 2011-2018 Lukas Martini
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

#include "gdt.h"
#include <stdint.h>
#include <string.h>
#include <mem/kmalloc.h>

#define SEG_DESCTYPE(x)  ((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x)      ((x) << 0x07) // Present
#define SEG_SAVL(x)      ((x) << 0x0C) // Available for system use
#define SEG_LONG(x)      ((x) << 0x0D) // Long mode
#define SEG_SIZE(x)      ((x) << 0x0E) // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)      ((x) << 0x0F) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x)     (((x) &  0x03) << 0x05)   // Set privilege level (0 - 3)

#define SEG_DATA_RD        0x00 // Read-Only
#define SEG_DATA_RDA       0x01 // Read-Only, accessed
#define SEG_DATA_RDWR      0x02 // Read/Write
#define SEG_DATA_RDWRA     0x03 // Read/Write, accessed
#define SEG_DATA_RDEXPD    0x04 // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   0x05 // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  0x06 // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX        0x08 // Execute-Only
#define SEG_CODE_EXA       0x09 // Execute-Only, accessed
#define SEG_CODE_EXRD      0x0A // Execute/Read
#define SEG_CODE_EXRDA     0x0B // Execute/Read, accessed
#define SEG_CODE_EXC       0x0C // Execute-Only, conforming
#define SEG_CODE_EXCA      0x0D // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     0x0E // Execute/Read, conforming
#define SEG_CODE_EXRDCA    0x0F // Execute/Read, conforming, accessed

#define GDT_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_CODE_EXRD

#define GDT_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_DATA_RDWR

#define GDT_CODE_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_CODE_EXRD

#define GDT_DATA_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_DATA_RDWR

extern void gdt_flush(void*);
extern void* stack_end;
static uint32_t* tss;

static struct pointer {
	// The upper 16 bits of all selector limits.
	uint16_t limit;
	void* base;
} __attribute__((packed)) pointer;
static uint64_t* descs;

static void create_descriptor(uint32_t num, uint32_t base, uint32_t limit, uint16_t flag) {
    // Create the high 32 bit segment
    descs[num]  =  limit       & 0x000F0000;         // set limit bits 19:16
    descs[num] |= (flag <<  8) & 0x00F0FF00;         // set type, p, dpl, s, g, d/b, l and avl fields
    descs[num] |= (base >> 16) & 0x000000FF;         // set base bits 23:16
    descs[num] |=  base        & 0xFF000000;         // set base bits 31:24

    // Shift by 32 to allow for low part of segment
    descs[num] <<= 32;

    // Create the low 32 bit segment
    descs[num] |= base  << 16;                       // set base bits 15:0
    descs[num] |= limit  & 0x0000FFFF;               // set limit bits 15:0
}

void gdt_set_tss(void* addr) {
	bzero(tss, 0x60);
	*(tss + 1) = (uint32_t)addr;
	*(tss + 2) = 0x10;

	create_descriptor(5, (uint32_t)tss, 0x60, 0x89);
	gdt_flush(&pointer);
}

void gdt_init() {
	descs = kmalloc(sizeof(uint64_t) * 6);
	pointer.limit = (sizeof(uint64_t) * 6) - 1;
	pointer.base = descs;

	create_descriptor(0, 0, 0, 0);
	create_descriptor(1, 0, 0xffffffff, GDT_CODE_PL0); // 0x08
	create_descriptor(2, 0, 0xffffffff, GDT_DATA_PL0); // 0x10
	create_descriptor(3, 0, 0xffffffff, GDT_CODE_PL3); // 0x1b
	create_descriptor(4, 0, 0xffffffff, GDT_DATA_PL3); // 0x23

 	tss = kmalloc(0x60);
	gdt_set_tss(&stack_end);
}
