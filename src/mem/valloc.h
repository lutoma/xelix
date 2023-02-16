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
#define VA_KERNEL (&valloc_kernel_ctx)

// Writable
#define VM_RW 1

// Readable by user space
#define VM_USER 2

// pfree() physical memory after context/range is unmapped
#define VM_FREE 4

/* This flag is used internally in task code to indicate the range should be
 * copied in fork() commands
 */
#define VM_TFORK 8

// Do not use Copy-on-Write for copies of this range
#define VM_NOCOW 16

// Zero out address space after allocation
#define VM_ZERO 32

// Temp hack
#define VM_NO_MAP 128

// vmap: Only map user-readable pages
#define VM_MAP_USER_ONLY 512

// temp
#define VM_MAP_UNDERALLOC_WORKAROUND 1024

#define VM_DEBUG 2048


#define valloc(ctx, vmem, size, phys, flags) valloc_at(ctx, vmem, size, NULL, phys, flags)

#define valloc_translate_ptr(range, inaddr, dir)   \
	(dir ? range->addr : range->phys)              \
	+ (inaddr - (dir ? range->phys : range->addr))


struct valloc_mem;
struct valloc_ctx {
	spinlock_t lock;
	uint32_t bitmap_data[bitmap_size(VALLOC_BITMAP_SIZE)];
	struct bitmap bitmap;
	struct valloc_mem* ranges;

	// Address of the actual page tables that will be read by the hardware
	struct paging_context* page_dir;
	struct paging_context* page_dir_phys;
};

struct valloc_mem_shard {
	struct valloc_mem_shard* next;
	size_t size;
	void* addr;
	void* phys;
};

typedef struct valloc_mem {
	struct valloc_mem* next;
	struct valloc_mem* previous;

	// Used in vfree
	struct valloc_mem* self;
	struct valloc_ctx* ctx;
	void* addr;
	size_t size;
	int flags;

	// Physical memory shards. If NULL, memory is contiguous
	struct valloc_mem_shard* shards;

	// For contiguous memory, this contains the physical address. NULL for sharded memory
	void* phys;
} vmem_t;


extern struct valloc_ctx valloc_kernel_ctx;

int valloc_at(struct valloc_ctx* ctx, vmem_t* vmem, size_t size, void* virt_request, void* phys, int flags);

int vm_alloc_many(int num, struct valloc_ctx** mctx, vmem_t** mvmem, size_t size, void* phys, int* mflags);

void* vmap(struct valloc_ctx* ctx, vmem_t* vmem, struct valloc_ctx* src_ctx,
	void* src_addr, size_t size, int flags);

vmem_t* valloc_get_range(struct valloc_ctx* ctx, void* addr, bool phys);
int vm_copy(struct valloc_ctx* dest, vmem_t* vmem_dest, vmem_t* vmem_src);
int vm_clone(struct valloc_ctx* dest, struct valloc_ctx* src);
int vfree(vmem_t* range);
int valloc_new(struct valloc_ctx* ctx, struct paging_context* page_dir);
void valloc_cleanup(struct valloc_ctx* ctx);
void* valloc_get_page_dir(struct valloc_ctx* ctx);
int valloc_stats(struct valloc_ctx* ctx, uint32_t* total, uint32_t* used);

static inline void* valloc_translate(struct valloc_ctx* ctx, void* raddress, bool phys) {
	vmem_t* range = valloc_get_range(ctx, raddress, phys);
	if(!range) {
		return 0;
	}
	return valloc_translate_ptr(range, raddress, phys);
}
