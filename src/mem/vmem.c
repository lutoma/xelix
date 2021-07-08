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
#include <mem/kmalloc.h>
#include <mem/mem.h>
#include <mem/paging.h>
#include <panic.h>
#include <string.h>
#include <tasks/scheduler.h>

static struct vmem_context* kernel_ctx = NULL;

/* Used in interrupt handlers to return to kernel paging context */
void* vmem_kernel_hwdata UL_VISIBLE("bss");

struct vmem_range* vmem_map(struct vmem_context* ctx, void* virt, void* phys, size_t size, int flags) {
	ctx = ctx ? ctx : kernel_ctx;
	if(unlikely(!ctx || ((flags & VM_COW) && !phys))) {
		return NULL;
	}

	struct vmem_range* range = zmalloc(sizeof(struct vmem_range));
	range->flags = flags;
	range->size = ALIGN(size, PAGE_SIZE);

	if(!phys && !(flags & VM_AOW)) {
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
		paging_set_range(ctx->hwdata, range);
	}

	return range;
}

struct vmem_range* vmem_get_range(struct vmem_context* ctx, void* addr, bool phys) {
	if(!ctx) {
		ctx = kernel_ctx;
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

#if 0
int vmem_page_fault_cb(task_t* task, void* addr) {
	struct vmem_range* range = vmem_get_range(task->vmem_ctx, addr, false);
	if(!range || !(range->flags & (VM_COW | VM_AOW))) {
		serial_printf("%s: no range for %#x\n", task->binary_path, addr);
		return -1;
	}

	// FIXME check if all other refs of the phys page have gone

	// FIXME only map the requested page, not entire range
	void* page = palloc(range->size / PAGE_SIZE);
	serial_printf("%s: have range %#x -> %#x size %#x, new location %#x\n",
		task->binary_path, range->virt_addr, range->phys_addr, range->size, page);

	memcpy(page, range->phys_addr, range->size);
	range->phys_addr = page;
	range->flags = range->cow_flags;
	paging_set_range(task->vmem_ctx->hwdata, range);
	return 0;
}
#endif

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
		ctx->hwdata = zpalloc(1);
		struct vmem_range* range = ctx->ranges;

		for(; range; range = range->next) {
			paging_set_range(ctx->hwdata, range);
		}
	}
	return ctx->hwdata;
}

void vmem_init() {
	// Initialize kernel context
	kernel_ctx = zmalloc(sizeof(struct vmem_context));
	vmem_map_flat(NULL, KERNEL_START, 0xfffff000, VM_RW);
	vmem_kernel_hwdata = vmem_get_hwdata(kernel_ctx);
	paging_init(vmem_kernel_hwdata);
}
