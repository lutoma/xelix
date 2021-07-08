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

#include "page_alloc.h"
#include <mem/paging.h>
#include <boot/multiboot.h>
#include <string.h>
#include <bitmap.h>
#include <panic.h>
#include <spinlock.h>

void* mem_page_alloc(struct mem_page_alloc_ctx* ctx, uint32_t size) {
	if(!spinlock_get(&ctx->lock, -1)) {
		return NULL;
	}

	uint32_t num = bitmap_find(&ctx->bitmap, size);
	bitmap_set(&ctx->bitmap, num, size);
	spinlock_release(&ctx->lock);
	return (void*)(num * PAGE_SIZE);
}

int mem_page_alloc_at(struct mem_page_alloc_ctx* ctx, void* addr, uint32_t size) {
	if(!spinlock_get(&ctx->lock, -1)) {
		return -1;
	}

	bitmap_set(&ctx->bitmap, (uintptr_t)addr / PAGE_SIZE, size);
	spinlock_release(&ctx->lock);
	return 0;
}

int mem_page_free(struct mem_page_alloc_ctx* ctx, uint32_t num, uint32_t size) {
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
	ctx->bitmap.first_free = 0;
	bitmap_clear_all(&ctx->bitmap);

	// Block NULL page
	bitmap_set(&ctx->bitmap, 0, 1);
	return 0;
}
