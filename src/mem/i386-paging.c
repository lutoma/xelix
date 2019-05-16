/* i386-paging.c: x86 paging stub for vmem
 * Copyright © 2011-2019 Lukas Martini
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

void paging_set_range(struct paging_context* ctx, struct vmem_range* range) {
	for(uintptr_t off = 0; off < range->length; off += PAGE_SIZE) {
		uintptr_t virt_addr = range->virt_start + off;
		uintptr_t phys_addr = range->phys_start + off;

		uint32_t page_dir_offset = virt_addr >> 22;
		uint32_t page_table_offset = (virt_addr >> 12) % 1024;

		struct page* page_dir = &(ctx->dir_entries[page_dir_offset]);

		struct page* page_table;
		if(!page_dir->present) {
			page_table = zmalloc_a(sizeof(struct page) * 1024);

			page_dir->present = true;
			page_dir->rw = 1;
			page_dir->user = 1;
			page_dir->frame = (uintptr_t)page_table >> 12;
		} else {
			page_table = (struct page*)(page_dir->frame << 12);
		}

		struct page* page = page_table  + page_table_offset;
		page->present = 1;
		page->rw = !range->readonly;
		page->user = range->user;
		page->frame = phys_addr >> 12;
	}
}

void paging_rm_context(struct paging_context* ctx) {
	for(int i = 0; i < 1024; i++) {
		if(ctx->dir_entries[i].present) {
			kfree((void*)(ctx->dir_entries[i].frame << 12));
		}
	}
	kfree(ctx);
}

void paging_init() {
	log(LOG_INFO, "vmem: Enabling paging bit, cr3=0x%x\n", vmem_kernel_hwdata);
	asm volatile(
		"cli;"
		"mov %0, %%cr3;"
		"mov %%cr0, %%eax;"
		"or $0x80000000, %%eax;"
		"mov %%eax, %%cr0;"
		"sti;"
	:: "r"(vmem_kernel_hwdata) : "memory", "eax");
}
