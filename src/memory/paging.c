/* paging.c: Paging intialization / allocation
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include "paging.h"
#include <lib/generic.h>
#include <memory/kmalloc.h>
#include <lib/log.h>

typedef struct
{
	bool present		: 1;	// Page present in memory
	bool rw				: 1;	// Read-only if clear, readwrite if set
	bool user			: 1;	// Supervisor level only if clear
	bool accessed		: 1;	// Has the page been accessed since last refresh?
	bool dirty			: 1;	// Has the page been written to since last refresh?
	uint32_t unused		: 7;	// Unused / reserved
	uint32_t frame		: 20;	// Frame address (shifted right 12 bits)
} page_t;

typedef struct
{
	page_t pages[1024];
} page_table_t;

void* pageDirectory = NULL;
page_table_t tables[1024];
void* tablesPhysical[1024];

int paging_assign(uint32_t virtual, uint32_t physical, bool rw, bool user)
{
	page_t page = tables[virtual / (1024 * 4)].pages[virtual % (1024 * 4) - 1];

	if(page.present)
		return 1;

	page.present = true;
	page.rw = rw;
	page.user = user;
	page.frame = physical >> 12;

	return 0;
}

void paging_init()
{
	memset(tables, 0, sizeof(page_table_t) * 1024);
	pageDirectory = &tables;
	
	// Map all the memory that has been kmalloc'd so far
	for(int i = 1; i < (kmalloc_getMemoryPosition() / 4); i++)
		paging_assign(i, i, 1, 0);
	
	// Write the address of our page directory to cr3
	asm volatile("mov cr3, %0":: "r"(&tablesPhysical));

	// Get the value of cr0
	uint32_t cr0;
	asm volatile("mov %0, cr0": "=r"(cr0));

	// Enable the paging bit
	cr0 |= 0x80000000;
	//asm volatile("mov cr0, %0":: "r"(cr0));
}
