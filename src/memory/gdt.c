/* gdt.c: Disable segmentation by defining one large segment
 * Copyright © 2010 Christoph Sünderhauf
 * Copyright © 2011 Lukas Martini
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

#include <lib/log.h>

/* For internal use only */
typedef struct
{
	uint16_t lowLimit; // The lower 16 bits of the limit.
	uint16_t lowBase; // The lower 16 bits of the base.
	uint8_t  middleBase; // The next 8 bits of the base.
	uint8_t  access; // Access flags, determine what ring this segment can be used in.
	uint8_t  granularity;
	uint8_t  highBase; // The last 8 bits of the base.
} __attribute__((packed))
entry_t;

typedef struct
{
	uint16_t limit; // The upper 16 bits of all selector limits.
	uint32_t base; // The address of the first gdt_entry_t struct.
} __attribute__((packed))
pointer_t;

// Defined in gdt.asm
extern void gdt_flush(uint32_t);

static entry_t entries[5];
static pointer_t pointer;

// Set the value of one GDT entry.
static void setGate(int32_t num, uint32_t base, uint32_t limit,
					uint8_t access, uint8_t gran)
{
	if(num > 4)
	{
		log(LOG_ERR, "gdt: Trying to set invalid gate %d.\n", num);
		return;
	}
	
	entries[num].lowBase    = (base & 0xFFFF);
	entries[num].middleBase = (base >> 16) & 0xFF;
	entries[num].highBase   = (base >> 24) & 0xFF;

	entries[num].lowLimit   = (limit & 0xFFFF);
	entries[num].granularity = (limit >> 16) & 0x0F;

	entries[num].granularity |= gran & 0xF0;
	entries[num].access = access;
}

// Initialisation routine - zeroes all the interrupt service routines,
// initialises the GDT and IDT.
void gdt_init()
{
	pointer.limit = (sizeof(entry_t) * 5) - 1;
	pointer.base  = (uint32_t)&entries;

	setGate(0, 0, 0, 0, 0); // Null segment
	setGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // 0x04: User mode Code segment
	setGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // 0x08: Kernel Code segment
	setGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // 0x0C: User mode Data segment
	setGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // 0x10: Kernel Data segment

	gdt_flush((uint32_t)&pointer);
}
