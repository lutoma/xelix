/* paging.c: Paging intialization / allocation
 * Copyright © 2011 Lukas Martini, Fritz Grimpen
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
page_table_t *page_tables1;
page_directory_t *page_directory1;

page_table_t *page_tables2;
page_directory_t *page_directory2;

page_table_t *current_page_tables;
page_directory_t *current_page_directory;

int paging_assign(uint32_t virtual, uint32_t physical, bool rw, bool user, bool mapped)
{
	// Align virtual and physical addresses to 4096 byte
	virtual = virtual - virtual % 4096;
	physical = physical - physical % 4096;

	uint32_t page_dir_offset = virtual >> 22;
	uint32_t page_table_offset = (virtual >> 12) % 1024;

	page_t *page_dir = &(current_page_directory->nodes[page_dir_offset]);
	page_t *page_table = &(current_page_tables[page_dir_offset].nodes[page_table_offset]);
	if (!page_dir->present)
	{
		page_dir->present = true;
		page_dir->rw = 1;
		page_dir->user = 1;
		page_dir->frame = (int)page_table >> 12;
	}

	if (page_table->present)
		return 1;

	page_table->present = mapped;
	page_table->rw = rw;
	page_table->user = user;
	page_table->frame = physical >> 12;

	return 0;
}

void paging_applyPage(struct vm_page *pg)
{
	bool user = true;
	bool write = !pg->readonly;

	if (pg->section == VM_SECTION_KERNEL && write == true)
		user = false;
	else if (pg->section == VM_SECTION_STACK && pg->allocated == 0)
		return;

	if (pg->section == VM_SECTION_UNMAPPED)
		paging_assign((uint32_t)pg->virt_addr, (uint32_t)pg->phys_addr, write, user, false);
	else
		paging_assign((uint32_t)pg->virt_addr, (uint32_t)pg->phys_addr, write, user, true);
}

static void paging_vmIterator(struct vm_context *ctx, struct vm_page *pg, uint32_t offset)
{
	paging_applyPage(pg);
}

int paging_apply(struct vm_context *ctx)
{
	memset(current_page_tables, 0, sizeof(page_table_t) * 1024);
	memset(current_page_directory, 0, sizeof(page_directory_t));
	vm_iterate(ctx, paging_vmIterator);

	vm_currentContext = ctx;

	asm volatile("mov cr3, %0":: "r"(&(current_page_directory->nodes)));

	if (current_page_tables == page_tables1)
		current_page_tables = page_tables2;
	else if (current_page_tables == page_tables2)
		current_page_tables = page_tables1;

	if (current_page_directory == page_directory1)
		current_page_directory = page_directory2;
	else if (current_page_directory == page_directory2)
		current_page_directory = page_directory1;

	return true;
}

void paging_init()
{
	vm_applyPage = paging_applyPage;

	page_tables1 = kmalloc_a(4194304);
	page_tables2 = kmalloc_a(4194304);
	current_page_tables = page_tables1;

	page_directory1 = kmalloc_a(4096);
	page_directory2 = kmalloc_a(4096);
	current_page_directory = page_directory1;

	paging_apply(vm_kernelContext);

	// Get the value of cr0
	uint32_t cr0;
	asm volatile("mov %0, cr0": "=r"(cr0));

	// Enable the paging bit
	cr0 |= 0x80000000;
	asm volatile("mov cr0, %0":: "r"(cr0));	
}
