#pragma once

/* Copyright © 2020-2022 Lukas Martini
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


#define VALLOC_BITMAP_SIZE 0xfffff000 / PAGE_SIZE
#define VA_KERNEL &valloc_kernel_ctx

// Writable
#define VA_RW 1

// Readable by user space
#define VA_USER 2

// pfree() physical memory after context/range is unmapped
#define VA_FREE 4

/* This flag is used internally in task code to indicate the range should be
 * copied in fork() commands
 */
#define VA_TFORK 8

// Allocate on write
#define VA_AOW 16

// Copy on write - implies VM_AOW
#define VA_COW 32

#define VA_NOCOW 64

// Temp hack
#define VA_NO_MAP 128

struct valloc_mem;
struct valloc_ctx {
	spinlock_t lock;
	uint32_t bitmap_data[bitmap_size(VALLOC_BITMAP_SIZE)];
	struct bitmap bitmap;
	struct valloc_mem* ranges;
};


typedef struct valloc_mem {
	struct valloc_ctx* ctx;
	void* addr;
	void* phys;
	size_t size;
} vmem_t;


extern struct valloc_ctx valloc_kernel_ctx;

int valloc(struct valloc_ctx* ctx, vmem_t* vmem, size_t size, void* phys, int flags);
int valloc_at(struct valloc_ctx* ctx, vmem_t* vmem, size_t size, void* addr, void* phys, int flags);
int vfree(struct valloc_ctx* ctx, uint32_t num, size_t size);
int valloc_stats(struct valloc_ctx* ctx, uint32_t* total, uint32_t* used);
int valloc_new(struct valloc_ctx* ctx);

static inline void* zvalloc(struct valloc_ctx* ctx, vmem_t* vmem, size_t size, void* phys, int flags) {
	if(valloc(ctx, vmem, size, phys, flags) == 0) {
		bzero(vmem->addr, vmem->size);
	}

	return 0;
}
