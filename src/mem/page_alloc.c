/* page_alloc.c: Physical memory page allocator
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

#include "page_alloc.h"
#include <mem/paging.h>
#include <boot/multiboot.h>
#include <string.h>
#include <bitmap.h>
#include <panic.h>
#include <spinlock.h>

// FIXME This code should be incorporated into vm.c, which is the only place that uses it.

void* mem_page_alloc(struct mem_page_alloc_ctx* ctx, size_t size) {
	if(!spinlock_get(&ctx->lock, -1)) {
		return NULL;
	}

	uint32_t num = bitmap_find(&ctx->bitmap, 0, size);
	if(num == -1) {
		return NULL;
	}

	bitmap_set(&ctx->bitmap, num, size);
	spinlock_release(&ctx->lock);
	return (void*)(num * PAGE_SIZE);
}

int mem_page_alloc_at(struct mem_page_alloc_ctx* ctx, void* addr, size_t size) {
	if(!spinlock_get(&ctx->lock, -1)) {
		return -1;
	}

	// FIXME Add optional? check for duplicate allocations
	bitmap_set(&ctx->bitmap, (uintptr_t)addr / PAGE_SIZE, size);
	spinlock_release(&ctx->lock);
	return 0;
}

int mem_page_free(struct mem_page_alloc_ctx* ctx, uint32_t num, size_t size) {
	// FIXME Add optional debug check if allocation even exists
	bitmap_clear(&ctx->bitmap, num, size);
	return 0;
}

int mem_page_alloc_stats(struct mem_page_alloc_ctx* ctx, uint32_t* total, uint32_t* used) {
	*total = ctx->bitmap.size * PAGE_SIZE;
	*used = bitmap_count(&ctx->bitmap) * PAGE_SIZE;
	return 0;
}

int mem_page_alloc_new(struct mem_page_alloc_ctx* ctx) {
	ctx->lock = 0;
	ctx->bitmap.data = ctx->bitmap_data;
	ctx->bitmap.size = PAGE_ALLOC_BITMAP_SIZE;
	bitmap_clear_all(&ctx->bitmap);

	// Block NULL page
	bitmap_set(&ctx->bitmap, 0, 1);
	return 0;
}
