/* i386-paging.c: x86 paging
 * Copyright © 2011-2021 Lukas Martini
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
#include <mem/mem.h>
#include <log.h>
#include <string.h>
#include <mem/vmem.h>
#include <int/int.h>

// Used in interrupt handlers to return to kernel paging context
struct paging_context* paging_kernel_ctx UL_VISIBLE("bss");
void* paging_alloc_end = KERNEL_END;

/*
void* paging_translate_to_phys(struct paging_context* ctx, void* virt) {
	uint32_t page_dir_offset = virt >> 22;
	uint32_t page_table_offset = (virt >> 12) % 1024;

	struct page* page_dir = &(ctx->dir_entries[page_dir_offset]);
	struct page* page_table = page_dir->frame << 12;
	struct page* page = page_table + page_table_offset;
	return page->frame << 12;
}
*/

void paging_set_range(struct paging_context* ctx, void* virt_addr, void* phys_addr, size_t size, int flags) {
	for(uintptr_t off = 0; off < size; off += PAGE_SIZE) {
		uintptr_t current_virt = (uintptr_t)virt_addr + off;

		uint32_t page_dir_offset = current_virt >> 22;
		uint32_t page_table_offset = (current_virt >> 12) % 1024;

		struct page* page_dir = &(ctx->dir_entries[page_dir_offset]);

		void* phys_table;
		struct page* page_table;

		if(!page_dir->present) {
			phys_table = palloc(1);

			// FIXME Possible chicken/egg problem here if valloc tries to allocate page within the page table we're trying to allocate
			page_table = zvalloc(1, phys_table, VM_RW);

			page_dir->present = true;
			page_dir->rw = 1;
			page_dir->user = 1;
			page_dir->frame = (uintptr_t)phys_table >> 12;
		} else {
			phys_table = (void*)(page_dir->frame << 12);

			if(phys_table < paging_alloc_end) {
				// Early page table allocation in kernel ctx, those are 1:1
				// mapped and cannot be translated by vmem_translate (yet)
				page_table = phys_table;
			} else {
				page_table = vmem_translate(NULL, phys_table, true);
			}
		}

		struct page* page = page_table + page_table_offset;
		page->present = 1;
		page->rw = flags & VM_RW;
		page->user = flags & VM_USER;
		page->frame = ((uintptr_t)phys_addr + off) >> 12;
		asm volatile("invlpg (%0)":: "r" (current_virt));

		// FIXME vfree page_table here?
	}
}

void paging_rm_context(struct paging_context* ctx) {
	for(int i = 0; i < 1024; i++) {
		if(ctx->dir_entries[i].present) {
			pfree((ctx->dir_entries[i].frame << 12) / PAGE_SIZE, 1);
		}
	}
	pfree((uintptr_t)ctx / PAGE_SIZE, 1);
}

void paging_init() {
	paging_kernel_ctx = ALIGN(KERNEL_END, PAGE_SIZE);
	bzero(paging_kernel_ctx, sizeof(struct paging_context));
	paging_alloc_end = (void*)paging_kernel_ctx + sizeof(struct paging_context);

	log(LOG_INFO, "paging: Building initial page directory at %#x\n", paging_kernel_ctx);

	/* Allocate page tables for the entire kernel virtual address space ahead
	 * of time. This is a bit wasteful since we're also allocating page tables
	 * for unused memory regions, but allocating these on the fly later is a
	 * gigantic headache: You could end up in a situation where the new
	 * location for the page table allocated by valloc lies within that page
	 * table itself, which would result in an endless loop between
	 * valloc/paging_set_range.
	 *
	 * Since this happens before memory allocation is ready, just store the
	 * data in free memory following KERNEL_END. This memory will later be
	 * reserved in mem.c.
	 */

	for(int i = 0; i < 1024; i++) {
		struct page* page_dir = &(paging_kernel_ctx->dir_entries[i]);
		page_dir->present = true;
		page_dir->rw = 1;
		page_dir->user = 0;
		page_dir->frame = (uintptr_t)paging_alloc_end >> 12;
		bzero(paging_alloc_end, PAGE_SIZE);
		paging_alloc_end += PAGE_SIZE;
	}

	log(LOG_INFO, "paging: Early page tables allocated up to %#x\n", paging_alloc_end);

	// Now, map the kernel binary and the page tables we just allocated in that page directory
	paging_set_range(paging_kernel_ctx, KERNEL_START, KERNEL_START,
		paging_alloc_end - KERNEL_START, VM_RW);

	asm volatile(
		"cli;"
		"mov %0, %%cr3;"
		"mov %%cr0, %%eax;"
		"or $0x80000000, %%eax;"
		"mov %%eax, %%cr0;"
		"sti;"
	:: "r"(paging_kernel_ctx) : "memory", "eax");

	log(LOG_INFO, "paging: Enabled\n");
}
