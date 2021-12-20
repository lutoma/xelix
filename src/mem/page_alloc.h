#pragma once

/* Copyright © 2020 Lukas Martini
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mem/paging.h>
#include <bitmap.h>
#include <string.h>
#include <stdint.h>
#include <spinlock.h>

#define PAGE_ALLOC_BITMAP_SIZE 0xfffff000 / PAGE_SIZE

struct mem_page_alloc_ctx {
	spinlock_t lock;
    uint32_t bitmap_data[bitmap_size(PAGE_ALLOC_BITMAP_SIZE)];
    struct bitmap bitmap;
};

void* mem_page_alloc(struct mem_page_alloc_ctx* ctx, size_t size);
int mem_page_alloc_at(struct mem_page_alloc_ctx* ctx, void* addr, size_t size);
int mem_page_free(struct mem_page_alloc_ctx* ctx, uint32_t num, size_t size);
int mem_page_alloc_stats(struct mem_page_alloc_ctx* ctx, uint32_t* total, uint32_t* used);
int mem_page_alloc_new(struct mem_page_alloc_ctx* ctx);
