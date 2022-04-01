/* valloc.c: Virtual memory allocator
 * Copyright © 2020-2022 Lukas Martini
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

#include "valloc.h"
#include <mem/paging.h>
#include <mem/kmalloc.h>
#include <mem/mem.h>
#include <boot/multiboot.h>
#include <string.h>
#include <bitmap.h>
#include <panic.h>
#include <spinlock.h>

static vmem_t malloc_ranges[50];
static int have_malloc_ranges = 50;

int valloc_at(struct valloc_ctx* ctx, vmem_t* vmem, size_t size, void* virt_request, void* phys, int flags) {
	if(!spinlock_get(&ctx->lock, -1)) {
		return -1;
	}

	// Allocate virtual address
	uint32_t page_num;
	void* virt;
	if(virt_request) {
		// FIXME Do duplicate checks
		virt = ALIGN_DOWN(virt_request, PAGE_SIZE);
		page_num = (uintptr_t)virt / PAGE_SIZE;
	} else {
		page_num = bitmap_find(&ctx->bitmap, size);

		if(page_num == -1) {
			spinlock_release(&ctx->lock);
			return -1;
		}

		virt = (void*)(page_num * PAGE_SIZE);
	}

	bitmap_set(&ctx->bitmap, page_num, size);

	// Allocate physical address if necessary
	if(!phys) {
		phys = palloc(size);
		if(!phys) {
			spinlock_release(&ctx->lock);
			return -1;
		}
	}

	// FIXME VM_ZERO should always map into kernel ctx to zero, not specified
	if((!(flags & VM_NO_MAP)) || flags & VM_ZERO) {
		if(ctx->page_dir) {
			paging_set_range(ctx->page_dir, virt, phys, size * PAGE_SIZE, flags);
		}
	}

	if(flags & VM_ZERO) {
		bzero(virt, size * PAGE_SIZE);
	}

	// FIXME Dealloc kernel mapping if both VM_NO_VIRT and VM_ZERO are set

	/* During initialization, kmalloc_init calls valloc once to get its memory
	 * space to allocate from. The zmalloc call below would fail since kmalloc
	 * is not ready yet. Another call to valloc can then happen in
	 * paging_set_range when a new page table is allocated.
	 * Add a dirty hack for that one-time special case.
	 */
	vmem_t* range;
	if(unlikely(!kmalloc_ready)) {
		if(likely(have_malloc_ranges)) {
			range = &malloc_ranges[50 - have_malloc_ranges--];
		} else {
			panic("valloc: preallocated ranges exhausted before kmalloc is ready\n");
		}
	} else {
		range = zmalloc(sizeof(vmem_t));
	}

	range->ctx = ctx;
	range->addr = virt;
	range->phys = phys;
	range->size = size * PAGE_SIZE;
	range->flags = flags;
	range->self = range;

	range->previous = NULL;
	range->next = ctx->ranges;
	if(ctx->ranges) {
		ctx->ranges->previous = range;
	}
	ctx->ranges = range;

	if(vmem) {
		memcpy(vmem, range, sizeof(vmem_t));
	}

	spinlock_release(&ctx->lock);
	return 0;
}

int vfree(vmem_t* range) {
	struct valloc_ctx* ctx = range->ctx;
	if(!spinlock_get(&ctx->lock, -1)) {
		return -1;
	}

	if(ctx->ranges == range->self) {
		ctx->ranges = range->next;
	}

	if(range->next) {
		range->next->previous = range->previous;
	}

	if(range->previous) {
		range->previous->next = range->next;
	}

	bitmap_clear(&ctx->bitmap, (uintptr_t)range->addr / PAGE_SIZE, RDIV(range->size, PAGE_SIZE));
	paging_clear_range(ctx->page_dir, range->addr, range->size);

	if(range->flags & VM_FREE) {
		pfree((uintptr_t)range->phys / PAGE_SIZE, RDIV(range->size, PAGE_SIZE));
	}

	kfree(range->self);
	spinlock_release(&ctx->lock);
	return 0;
}

vmem_t* valloc_get_range(struct valloc_ctx* ctx, void* addr, bool phys) {
	if(!spinlock_get(&ctx->lock, -1)) {
		return NULL;
	}

	if(!phys && !bitmap_get(&ctx->bitmap, (uintptr_t)addr / PAGE_SIZE)) {
		spinlock_release(&ctx->lock);
		return NULL;
	}

	vmem_t* range = ctx->ranges;
	for(; range; range = range->next) {
		void* start = (phys ? range->phys : range->addr);
		if(addr >= start && addr < (start + range->size)) {
			spinlock_release(&ctx->lock);
			return range;
		}
	}

	spinlock_release(&ctx->lock);
	return NULL;
}

int valloc_new(struct valloc_ctx* ctx, struct paging_context* page_dir) {
	ctx->lock = 0;
	ctx->ranges = NULL;
	ctx->bitmap.data = ctx->bitmap_data;
	ctx->bitmap.size = PAGE_ALLOC_BITMAP_SIZE;
	bitmap_clear_all(&ctx->bitmap);

	// Don't allocate null pointer
	bitmap_set(&ctx->bitmap, 0, 1);

	if(page_dir) {
		ctx->page_dir = page_dir;
		ctx->page_dir_phys = page_dir;
	} else {
	/*	vmem_t vmem;
		valloc(VA_KERNEL, &vmem, 1, NULL, VM_RW | VM_ZERO);
		ctx->page_dir = vmem.addr;
		ctx->page_dir_phys = vmem.phys;
	*/
	}

	// Block NULL page
	bitmap_set(&ctx->bitmap, 0, 1);
	return 0;
}

void valloc_cleanup(struct valloc_ctx* ctx) {
	if(ctx->page_dir) {
		paging_rm_context(ctx->page_dir);
	}

	vmem_t* range = ctx->ranges;
	while(range) {
		if(range->flags & VM_FREE) {
			pfree((uintptr_t)range->phys / PAGE_SIZE, RDIV(range->size, PAGE_SIZE));
		}

		vmem_t* old_range = range;
		range = range->next;
		kfree(old_range);
	}
}

void* valloc_get_page_dir(struct valloc_ctx* ctx) {
	if(!ctx->page_dir) {
		vmem_t vmem;
		valloc(VA_KERNEL, &vmem, 1, NULL, VM_RW | VM_ZERO);
		ctx->page_dir = vmem.addr;
		ctx->page_dir_phys = vmem.phys;

		vmem_t* range = ctx->ranges;

		for(; range; range = range->next) {
			paging_set_range(ctx->page_dir, range->addr, range->phys, range->size, range->flags);
		}
	}
	return ctx->page_dir_phys;
}

int valloc_stats(struct valloc_ctx* ctx, uint32_t* total, uint32_t* used) {
	*total = ctx->bitmap.size * PAGE_SIZE;
	*used = bitmap_count(&ctx->bitmap) * PAGE_SIZE;
	return 0;
}
