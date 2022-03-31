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

struct vmem_range* vmem_map(struct vmem_context* ctx, void* virt, void* phys, size_t size, int flags) {
	if(!ctx) {
		panic("no ctx in vmem_map");
	}

	if(unlikely(!ctx || ((flags & VM_COW) && !phys))) {
		return NULL;
	}

	struct vmem_range* range = zmalloc(sizeof(struct vmem_range));

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
		panic("no ctx in vmem_get_range");
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
		vmem_t vmem;
		valloc(VA_KERNEL, &vmem, 1, NULL, VM_RW | VM_ZERO);
		ctx->hwdata = vmem.addr;
		ctx->hwdata_phys = vmem.phys;

		struct vmem_range* range = ctx->ranges;

		for(; range; range = range->next) {
			paging_set_range(ctx->hwdata, range->virt_addr, range->phys_addr, range->size, range->flags);
		}
	}
	return ctx->hwdata_phys;
}
