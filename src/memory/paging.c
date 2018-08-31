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
#include <memory/kmalloc.h>
#include <log.h>
#include <print.h>
#include <string.h>
#include <memory/vmem.h>
#include <hw/interrupts.h>

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

struct paging_context {
	page_directory_t directory;
	page_table_t tables[1024];
};

static bool paging_initialized = false;

static int paging_assign(struct paging_context *ctx, uint32_t virtual, uint32_t physical, bool rw, bool user, bool mapped)
{
	// Align virtual and physical addresses to 4096 byte
	virtual = virtual - virtual % 4096;
	physical = physical - physical % 4096;

	uint32_t page_dir_offset = virtual >> 22;
	uint32_t page_table_offset = (virtual >> 12) % 1024;

	page_t *page_dir = &(ctx->directory.nodes[page_dir_offset]);
	page_t *page_table = &(ctx->tables[page_dir_offset].nodes[page_table_offset]);
	if (!page_dir->present)
	{
		page_dir->present = true;
		page_dir->rw = 1;
		page_dir->user = 1;
		page_dir->frame = (int)page_table >> 12;
	}

	page_table->present = mapped;
	page_table->rw = rw;
	page_table->user = user;
	page_table->frame = physical >> 12;

	return 0;
}

void paging_applyPage(struct vmem_context *ctx, struct vmem_page *pg)
{
	struct paging_context *pgCtx = vmem_get_cache(ctx);
	bool user = true;
	bool write = !pg->readonly;

	if (pg->section == VMEM_SECTION_KERNEL && write == true)
		user = false;
	else if (pg->section == VMEM_SECTION_STACK && pg->allocated == 0)
		return;

	paging_assign(pgCtx, (uint32_t)pg->virt_addr, (uint32_t)pg->phys_addr, write, user, (pg->section != VMEM_SECTION_UNMAPPED));
}

static void paging_vmIterator(struct vmem_context *ctx, struct vmem_page *pg, uint32_t offset)
{
	paging_applyPage(ctx, pg);
}

int paging_apply(struct vmem_context *ctx)
{
	if(!paging_initialized) {
		return false;
	}

	struct paging_context *pgCtx = vmem_get_cache(ctx);

	if (pgCtx == NULL)
	{
		/* Build paging context for vmem_context */
		pgCtx = kmalloc_a(sizeof(struct paging_context));
		if (pgCtx == NULL)
			return false;
		memset(pgCtx, 0, sizeof(struct paging_context));
		vmem_set_cache(ctx, pgCtx);

		vmem_iterate(ctx, paging_vmIterator);
	}

	vmem_currentContext = ctx;
	asm volatile("mov cr3, %0"::"r"(pgCtx));

	return true;
}

void paging_init()
{
	paging_initialized = true;
	vmem_applyPage = paging_applyPage;
	paging_apply(vmem_kernelContext);

	// Get the value of cr0
	uint32_t cr0;
	asm volatile("mov %0, cr0": "=r"(cr0));

	// Enable the paging bit
	cr0 |= 0x80000000;
	asm volatile("mov cr0, %0":: "r"(cr0));
}
