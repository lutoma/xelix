#pragma once

/* Copyright © 2020-2023 Lukas Martini
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

#define VM_BITMAP_SIZE 0xfffff000 / PAGE_SIZE
#define VM_KERNEL (&vm_kernel_ctx)

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

// vm_map: Only map user-readable pages
#define VM_MAP_USER_ONLY 512

// temp
#define VM_MAP_UNDERALLOC_WORKAROUND 1024

#define VM_DEBUG 2048


#define vm_alloc(ctx, vmem, size, phys, flags) vm_alloc_at(ctx, vmem, size, NULL, phys, flags)

#define valloc_translate_ptr(range, inaddr, dir)   \
	(dir ? range->addr : range->phys)              \
	+ (inaddr - (dir ? range->phys : range->addr))


struct vm_alloc;
struct vm_ctx {
	spinlock_t lock;
	uint32_t bitmap_data[bitmap_size(VM_BITMAP_SIZE)];
	struct bitmap bitmap;
	struct vm_alloc* ranges;

	// Address of the actual page tables that will be read by the hardware
	struct paging_context* page_dir;
	struct paging_context* page_dir_phys;
};

struct vm_alloc_shard {
	struct vm_alloc_shard* next;
	size_t size;
	void* addr;
	void* phys;
};

typedef struct vm_alloc {
	struct vm_alloc* next;
	struct vm_alloc* previous;

	// Used in vm_free
	struct vm_alloc* self;
	struct vm_ctx* ctx;
	void* addr;
	size_t size;
	int flags;

	// Physical memory shards. If NULL, memory is contiguous
	struct vm_alloc_shard* shards;

	// For contiguous memory, this contains the physical address. NULL for sharded memory
	void* phys;
} vm_alloc_t;


extern struct vm_ctx vm_kernel_ctx;

void* vm_alloc_at(struct vm_ctx* ctx, vm_alloc_t* vmem, size_t size,
	void* virt_request, void* phys, int flags);

void* vm_alloc_many(int num, struct vm_ctx** mctx, vm_alloc_t** mvmem,
	size_t size, void* phys, int* mflags);

void* vm_map(struct vm_ctx* ctx, vm_alloc_t* vmem, struct vm_ctx* src_ctx,
	void* src_addr, size_t size, int flags);

vm_alloc_t* vm_get(struct vm_ctx* ctx, void* addr, bool phys);
int vm_copy(struct vm_ctx* dest, vm_alloc_t* vmem_dest, vm_alloc_t* vmem_src);
int vm_clone(struct vm_ctx* dest, struct vm_ctx* src);
int vm_free(vm_alloc_t* range);
int vm_new(struct vm_ctx* ctx, struct paging_context* page_dir);
void vm_cleanup(struct vm_ctx* ctx);
void* vm_pagedir(struct vm_ctx* ctx);
int vm_stats(struct vm_ctx* ctx, uint32_t* total, uint32_t* used);

// FIXME Deprecated
static inline void* valloc_translate(struct vm_ctx* ctx, void* raddress, bool phys) {
	vm_alloc_t* range = vm_get(ctx, raddress, phys);
	if(!range) {
		return 0;
	}
	return valloc_translate_ptr(range, raddress, phys);
}
