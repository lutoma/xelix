#pragma once

/* Copyright © 2019-2021 Lukas Martini
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

#include <mem/paging.h>
#include <string.h>
#include <mem/page_alloc.h>

extern struct mem_page_alloc_ctx mem_phys_alloc_ctx;
extern struct mem_page_alloc_ctx mem_virt_alloc_ctx;

#define palloc(size) (mem_page_alloc(&mem_phys_alloc_ctx, size))
//#define pfree(num, size) (mem_page_free(&mem_phys_alloc_ctx, num, size))
#define pfree(num, size)

void* valloc(size_t size, void* phys, int flags);
int valloc_at(size_t size, void* virt, void* phys, int flags);

static inline void* zvalloc(size_t size, void* phys, int flags) {
    void* buf = valloc(size, phys, flags);
    if(buf) {
        bzero(buf, size * PAGE_SIZE);
    }

    return buf;
}

void mem_init();
