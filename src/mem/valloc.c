/* page_alloc.c: Page allocator
 * Copyright © 2020 Lukas Martini
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

/* Generic page allocator used for physical and both kernel and task virtual
 * memory. Intentionally kept very simple for now relying on a single bitmap,
 * but can be extended later as required.
 */

#include "valloc.h"
#include <mem/paging.h>
#include <mem/mem.h>
#include <boot/multiboot.h>
#include <string.h>
#include <bitmap.h>
#include <panic.h>
#include <spinlock.h>


int valloc(struct valloc_ctx* ctx, vmem_t* vmem, size_t size, void* phys, int flags) {
	if(!spinlock_get(&ctx->lock, -1)) {
		return -1;
	}

	if(!phys) {
		phys = palloc(size);
		if(!phys) {
			return -1;
		}
	}

	uint32_t num = bitmap_find(&ctx->bitmap, size);
	bitmap_set(&ctx->bitmap, num, size);
	spinlock_release(&ctx->lock);

	void* virt = (void*)(num * PAGE_SIZE);
	if(!virt) {
		pfree((uintptr_t)phys / PAGE_SIZE, size);
		return -1;
	}

	if(!(flags & VM_VALLOC_NO_MAP)) {
		vmem_map(NULL, virt, phys, size * PAGE_SIZE, flags);
	}

	vmem->ctx = ctx;
	vmem->addr = virt;
	vmem->phys = phys;
	vmem->size = size * PAGE_SIZE;
	return 0;
}

int valloc_at(struct valloc_ctx* ctx, vmem_t* vmem, size_t size, void* addr, void* phys, int flags) {
	if(!spinlock_get(&ctx->lock, -1)) {
		return -1;
	}


	if(!phys) {
		phys = palloc(size);
		if(!phys) {
			return -1;
		}
	}

	// FIXME Add optional? check for duplicate allocations
	bitmap_set(&ctx->bitmap, (uintptr_t)addr / PAGE_SIZE, size);
	spinlock_release(&ctx->lock);
	vmem_map(NULL, addr, phys, size * PAGE_SIZE, flags);

	vmem->ctx = ctx;
	vmem->addr = addr;
	vmem->phys = phys;
	vmem->size = size * PAGE_SIZE;
	return 0;
}

int vfree(struct valloc_ctx* ctx, uint32_t num, size_t size) {
	// FIXME Add optional debug check if allocation even exists
	bitmap_clear(&ctx->bitmap, num, size);
	return 0;
}

int valloc_stats(struct valloc_ctx* ctx, uint32_t* total, uint32_t* used) {
	*total = ctx->bitmap.size * PAGE_SIZE;
	*used = bitmap_count(&ctx->bitmap) * PAGE_SIZE;
	return 0;
}

int valloc_new(struct valloc_ctx* ctx) {
	ctx->lock = 0;
	ctx->bitmap.data = ctx->bitmap_data;
	ctx->bitmap.size = PAGE_ALLOC_BITMAP_SIZE;
	bitmap_clear_all(&ctx->bitmap);

	// Block NULL page
	bitmap_set(&ctx->bitmap, 0, 1);
	return 0;
}
