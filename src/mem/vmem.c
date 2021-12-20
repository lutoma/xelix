/* vmem.c: Virtual memory management
 * Copyright © 2013-2020 Lukas Martini
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

#include "vmem.h"
#include <log.h>
#include <mem/mem.h>
#include <mem/paging.h>
#include <mem/kmalloc.h>
#include <panic.h>
#include <string.h>
#include <tasks/scheduler.h>

static struct vmem_context kernel_ctx;
static struct vmem_range malloc_ranges[50];
static int have_malloc_ranges = 50;

struct vmem_range* vmem_map(struct vmem_context* ctx, void* virt, void* phys, size_t size, int flags) {
	ctx = ctx ? ctx : &kernel_ctx;
	if(unlikely(!ctx || ((flags & VM_COW) && !phys))) {
		return NULL;
	}

	/* During initialization, kmalloc_init calls valloc once to get its memory
	 * space to allocate from. valloc calls this function to map the memory,
	 * but the zmalloc call below would fail since kmalloc is not ready yet.
	 * Another call to vmem_map  can then happen in paging_set_range when a
	 * new page table is allocated.
	 * Add a dirty hack for that one-time special case.
	 */
	struct vmem_range* range;
	if(likely(!have_malloc_ranges)) {
		if(likely(kmalloc_ready)) {
			range = zmalloc(sizeof(struct vmem_range));
		} else {
			panic("vmem: preallocated ranges exhausted before kmalloc is ready\n");
		}
	} else {
		range = &malloc_ranges[50 - have_malloc_ranges--];
	}

	range->flags = flags;
	range->size = ALIGN(size, PAGE_SIZE);

	if(!phys && !(flags & VM_AOW)) {
		// FIXME
		panic("vmem_map no phys");
		phys = palloc(range->size / PAGE_SIZE);
	}

	range->virt_addr = ALIGN_DOWN(virt, PAGE_SIZE);
	range->phys_addr = ALIGN_DOWN(phys, PAGE_SIZE);

	if(flags & (VM_COW | VM_AOW)) {
		range->cow_flags = flags & ~(VM_COW | VM_AOW);
		range->flags &= ~VM_RW;
	}

	range->next = ctx->ranges;
	ctx->ranges = range;

	if(ctx->hwdata) {
		paging_set_range(ctx->hwdata, range->virt_addr, range->phys_addr, range->size, range->flags);
	}

	return range;
}

struct vmem_range* vmem_get_range(struct vmem_context* ctx, void* addr, bool phys) {
	if(!ctx) {
		ctx = &kernel_ctx;
	}

	struct vmem_range* range = ctx->ranges;
	for(; range; range = range->next) {
		void* start = (phys ? range->phys_addr : range->virt_addr);

		if(addr >= start && addr <= (start + range->size)) {
			return range;
		}
	}
	return NULL;
}

void vmem_rm_context(struct vmem_context* ctx) {
	paging_rm_context(ctx->hwdata);

	struct vmem_range* range = ctx->ranges;
	while(range) {
		if(range->flags & VM_FREE &&
			(!range->ref_count || __sync_fetch_and_sub(range->ref_count, 1) <= 1)) {

			if(range->ref_count) {
				kfree(range->ref_count);
			}

			pfree((uintptr_t)range->phys_addr / PAGE_SIZE, range->size / PAGE_SIZE);
		}

		struct vmem_range* old_range = range;
		range = range->next;
		kfree(old_range);
	}

	kfree(ctx);
}

void* vmem_get_hwdata(struct vmem_context* ctx) {
	if(!ctx->hwdata) {
		ctx->hwdata_phys = palloc(1);
		ctx->hwdata = zvalloc(1, ctx->hwdata_phys, VM_RW);

		struct vmem_range* range = ctx->ranges;

		for(; range; range = range->next) {
			paging_set_range(ctx->hwdata, range->virt_addr, range->phys_addr, range->size, range->flags);
		}
	}
	return ctx->hwdata_phys;
}

void vmem_init() {
	kernel_ctx.hwdata = paging_kernel_ctx;
	kernel_ctx.hwdata_phys = paging_kernel_ctx;
	kernel_ctx.ranges = NULL;
}
