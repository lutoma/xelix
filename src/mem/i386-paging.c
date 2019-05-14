/* paging.c: x86 paging stub for vmem
 * Copyright © 2011 Fritz Grimpen
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include "paging.h"
#include <mem/kmalloc.h>
#include <log.h>
#include <string.h>
#include <mem/vmem.h>
#include <int/int.h>

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

void paging_apply(struct vmem_context* ctx, struct vmem_page *pg) {
	uint32_t page_dir_offset = (uintptr_t)pg->virt_addr >> 22;
	uint32_t page_table_offset = ((uintptr_t)pg->virt_addr >> 12) % 1024;

	struct paging_context* pg_ctx = ctx->tables;
	page_t *page_dir = &(pg_ctx->directory.nodes[page_dir_offset]);
	page_t *page_table = &(pg_ctx->tables[page_dir_offset].nodes[page_table_offset]);

	if(!page_dir->present) {
		page_dir->present = true;
		page_dir->rw = 1;
		page_dir->user = 1;
		page_dir->frame = (uintptr_t)page_table >> 12;
	}

	page_table->present = pg->allocated;
	page_table->rw = !pg->readonly;
	page_table->user = pg->user;
	page_table->frame = (uintptr_t)pg->phys_addr >> 12;
}

void* paging_get_table(struct vmem_context* ctx) {
	if(!ctx->tables) {
		/* Build paging context for vmem_context */
		ctx->tables = zmalloc_a(sizeof(struct paging_context));
		if(!ctx->tables) {
			return false;
		}

		struct vmem_page* page = ctx->first_page;
		for(int i = 0; page != NULL && i < ctx->pages; i++) {
			paging_apply(ctx, page);
			page = page->next;
		}
	}

	return ctx->tables;
}

void paging_init() {
	paging_kernel_cr3 = paging_get_table(vmem_kernelContext);

	log(LOG_INFO, "vmem: Enabling paging bit, cr3=0x%x\n", paging_kernel_cr3);
	asm volatile(
		"cli;"
		"mov %0, %%cr3;"
		"mov %%cr0, %%eax;"
		"or $0x80000000, %%eax;"
		"mov %%eax, %%cr0;"
		"sti;"
	:: "r"(paging_kernel_cr3) : "memory", "eax");
}
