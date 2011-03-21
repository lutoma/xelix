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
	uint16 lowLimit; // The lower 16 bits of the limit.
	uint16 lowBase; // The lower 16 bits of the base.
	uint8  middleBase; // The next 8 bits of the base.
	uint8  access; // Access flags, determine what ring this segment can be used in.
	uint8  granularity;
	uint8  highBase; // The last 8 bits of the base.
} __attribute__((packed))
entry_t;

typedef struct
{
	uint16 limit; // The upper 16 bits of all selector limits.
	uint32 base; // The address of the first gdt_entry_t struct.
} __attribute__((packed))
pointer_t;

// Defined in gdt.asm
extern void gdt_flush(uint32);

static entry_t entries[5];
static pointer_t pointer;

// Set the value of one GDT entry.
static void setGate(sint32 num, uint32 base, uint32 limit, uint8 access, uint8 gran)
{
	if(num > 4)
	{
		log("gdt: Warning: Trying to set invalid gate %d.\n", num);
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
	pointer.base  = (uint32)&entries;

	setGate(0, 0, 0, 0, 0); // Null segment
	setGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
	setGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
	setGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
	setGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

	gdt_flush((uint32)&pointer);
}
