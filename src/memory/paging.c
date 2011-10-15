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
#include <lib/print.h>
#include <lib/datetime.h>
#include <memory/vm.h>

typedef struct {
	bool present    : 1;
	bool rw         : 1;
	bool user       : 1;
	bool accessed   : 1;
	bool dirty      : 1;
	uint32_t unused : 7;
	uint32_t frame  : 20;
} page_t;

typedef struct {
	page_t nodes[1024];
} page_directory_t;

typedef struct {
	page_t nodes[1024];
} page_table_t;

// page_tables[PAGE_DIR]
page_table_t page_tables[1024];
page_directory_t page_directory;

int paging_assign(uint32_t virtual, uint32_t physical, bool rw, bool user)
{
	// Align virtual and physical addresses to 4096 byte
	virtual = virtual - virtual % 4096;
	physical = physical - physical % 4096;

	uint32_t page_dir_offset = virtual >> 22;
	uint32_t page_table_offset = (virtual >> 12) % 1024;

	page_t *page_dir = &(page_directory.nodes[page_dir_offset]);
	page_t *page_table = &(page_tables[page_dir_offset].nodes[page_table_offset]);
	if (!page_dir->present)
	{
		page_dir->present = true;
		page_dir->rw = rw;
		page_dir->user = user;
		page_dir->frame = (int)page_table >> 12;
	}

	if (page_table->present)
		return 1;

	page_table->present = true;
	page_table->rw = rw;
	page_table->user = user;
	page_table->frame = physical >> 12;

	return 0;
}

static void paging_vmIterator(struct vm_context *ctx, struct vm_page *pg, uint32_t offset)
{
	bool onlyRing0 = 0;
	if (pg->section == VM_SECTION_KERNEL && pg->readonly == 0)
		onlyRing0 = 1;

	paging_assign((uint32_t)pg->virt_addr, (uint32_t)pg->phys_addr, pg->readonly, onlyRing0);
}

int paging_apply(struct vm_context *ctx)
{
	vm_iterate(ctx, paging_vmIterator);
	return true;
}

void paging_init()
{
	paging_apply(vm_kernelContext);

	// Write the address of our page directory to cr3
	asm volatile("mov cr3, %0":: "r"(&page_directory.nodes));

	// Get the value of cr0
	uint32_t cr0;
	asm volatile("mov %0, cr0": "=r"(cr0));

	// Enable the paging bit
	cr0 |= 0x80000000;
	asm volatile("mov cr0, %0":: "r"(cr0));	
}
