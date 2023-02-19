/* vm.c: Virtual memory management
 * Copyright © 2020-2023 Lukas Martini
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

#include <mem/vm.h>
#include <mem/paging.h>
#include <mem/kmalloc.h>
#include <mem/mem.h>
#include <boot/multiboot.h>
#include <string.h>
#include <bitmap.h>
#include <panic.h>
#include <spinlock.h>

static vm_alloc_t malloc_ranges[50];
static int have_malloc_ranges = 50;

#ifdef CONFIG_VM_DEBUG
	#ifdef CONFIG_VM_DEBUG_ALL
		#define debug(args...) { log(LOG_DEBUG, args); }
	#else
		#define debug(args...) { if(flags & VM_DEBUG) { log(LOG_DEBUG, args); } }
	#endif
#else
	#define debug(...)
#endif

static inline vm_alloc_t* get_range(struct vm_ctx* ctx, void* addr, bool phys) {
	if(!phys && !bitmap_get(&ctx->bitmap, (uintptr_t)addr / PAGE_SIZE)) {
		return NULL;
	}

	vm_alloc_t* range = ctx->ranges;
	for(; range; range = range->next) {
		void* start = (phys ? range->phys : range->addr);
		if(addr >= start && addr < (start + range->size)) {
			return range;
		}
	}

	return NULL;
}

static inline void* alloc_virt(struct vm_ctx* ctx, size_t size, void* request) {
	uint32_t page_num;
	void* virt;
	if(request) {
		virt = ALIGN_DOWN(request, PAGE_SIZE);
		page_num = (uintptr_t)virt / PAGE_SIZE;

		for(int i = 0; i < size; i++) {
			if(bitmap_get(&ctx->bitmap, page_num + i)) {
				log(LOG_ERR, "vm: Duplicate allocation attempt in context %#x at %#x\n", ctx, (page_num + i) * PAGE_SIZE);

				vm_alloc_t* crange = get_range(ctx, (void*)((page_num + i) * PAGE_SIZE), false);
				if(crange) {
					log(LOG_ERR, "vm: Conflicting range: %#x - %#x\n", crange->addr, crange->addr + crange->size);
				}
				return NULL;
			}
		}
	} else {
		page_num = bitmap_find(&ctx->bitmap, 0, size);

		if(page_num == -1) {
			return NULL;
		}

		virt = (void*)(page_num * PAGE_SIZE);
	}

	bitmap_set(&ctx->bitmap, page_num, size);
	return virt;
}

vm_alloc_t* vm_get(struct vm_ctx* ctx, void* addr, bool phys) {
	if(!spinlock_get(&ctx->lock, -1)) {
		return NULL;
	}

	vm_alloc_t* range = get_range(ctx, addr, phys);
	spinlock_release(&ctx->lock);
	return range;
}

static inline vm_alloc_t* new_range() {
	/* During initialization, kmalloc_init calls vm_alloc once to get its
	 * memory space to allocate from. The zmalloc call below would fail since
	 * kmalloc is not ready yet. Another call to vm_alloc can then happen in
	 * paging_set_range when a new page table is allocated.
	 * Add a dirty hack for that one-time special case.
	 */

	vm_alloc_t* range;

	// FIXME combine into simple early_alloc with the initial page dir allocation
	if(unlikely(!kmalloc_ready)) {
		if(likely(have_malloc_ranges)) {
			range = &malloc_ranges[50 - have_malloc_ranges--];
		} else {
			panic("vm: preallocated ranges exhausted before kmalloc is ready\n");
		}
	} else {
		range = kmalloc(sizeof(vm_alloc_t));
	}

	if(range) {
		bzero(range, sizeof(vm_alloc_t));
		range->self = range;
	}
	return range;
}

static inline void* setup_phys(struct vm_ctx* ctx, size_t size, void* virt, void* phys, int flags) {
	// Allocate memory if needed
	if(!phys) {
		phys = palloc(size);
		if(!phys) {
			return NULL;
		}
	}

	if(!(flags & VM_NO_MAP)) {
		if(ctx->page_dir) {
			paging_set_range(ctx->page_dir, virt, phys, size * PAGE_SIZE, flags);
		}
	}

	if(flags & VM_ZERO) {
		if(ctx == VM_KERNEL && !(flags & VM_NO_MAP)) {
			bzero(virt, size * PAGE_SIZE);
		} else {
			/* If the allocation is not in the kernel context or is set as NO_MAP,
			 * temporarily map it into the kernel virtual address space to zero it.
			 */
			if(!spinlock_get(&VM_KERNEL->lock, -1)) {
				return NULL;
			}

			void* zero_addr = alloc_virt(VM_KERNEL, size, NULL);
			spinlock_release(&VM_KERNEL->lock);
			if(zero_addr == NULL) {
				return NULL;
			}

			paging_set_range(VM_KERNEL->page_dir, zero_addr, phys, size * PAGE_SIZE, VM_RW);
			bzero(zero_addr, size * PAGE_SIZE);
			paging_clear_range(VM_KERNEL->page_dir, zero_addr, size * PAGE_SIZE);
			bitmap_clear(&VM_KERNEL->bitmap, (uintptr_t)zero_addr / PAGE_SIZE, RDIV(size, PAGE_SIZE));
		}
	}

	return phys;
}

void* vm_alloc_at(struct vm_ctx* ctx, vm_alloc_t* vmem, size_t size, void* virt_request, void* phys, int flags) {
	// FIXME Fail if size, virt_request or phys are not page aligned?

	if(!spinlock_get(&ctx->lock, -1)) {
		return NULL;
	}

	// Allocate virtual address
	void* virt = alloc_virt(ctx, size, virt_request);
	spinlock_release(&ctx->lock);
	if(!virt) {
		return NULL;
	}

	debug("ctx %p page_num %5d vm_alloc_at %p size %#x\n", ctx, page_num, page_num * PAGE_SIZE, size * PAGE_SIZE);
	phys = setup_phys(ctx, size, virt, phys, flags);

	if(!spinlock_get(&ctx->lock, -1)) {
		return NULL;
	}

	vm_alloc_t* range = new_range();
	if(!range) {
		return NULL;
	}

	range->ctx = ctx;
	range->addr = virt;
	range->phys = phys;
	range->size = size * PAGE_SIZE;
	range->flags = flags;
	range->next = ctx->ranges;

	if(ctx->ranges) {
		ctx->ranges->previous = range;
	}
	ctx->ranges = range;

	if(vmem) {
		memcpy(vmem, range, sizeof(vm_alloc_t));
	}

	debug("ctx %#x vm_alloc_at %p -> %p size %#x\n", ctx, range->addr, range->phys, range->size);
	spinlock_release(&ctx->lock);
	return range->addr;
}

void* vm_alloc_many(int num, struct vm_ctx** mctx, vm_alloc_t** mvmem, size_t size, void* phys, int* mflags) {
	for(int i = 0; i < num; i++) {
		if(!spinlock_get(&mctx[i]->lock, -1)) {
			for(int i = i - 1; i >= 0; i++) {
				spinlock_release(&mctx[i]->lock);
			}
			return NULL;
		}
	}

	uint32_t page_num = 0;
	// Try to find a matching allocation in all contexts
	while(true) {
		page_num = bitmap_find(&mctx[0]->bitmap, page_num, size);
		if(page_num == -1) {
			goto release_and_fail;
		}

		bool all_free = true;
		for(int i = 1; i < num; i++) {
			if(bitmap_get_range(&mctx[i]->bitmap, page_num, size)) {
				all_free = false;
				page_num++;
				break;
			}
		}

		if(all_free) {
			break;
		}
	}

	void* virt = (void*)(page_num * PAGE_SIZE);

	for(int i = 0; i < num; i++) {
		struct vm_ctx* lctx = mctx[i];

		bitmap_set(&lctx->bitmap, page_num, size);
		phys = setup_phys(lctx, size, virt, phys, mflags[i]);

		vm_alloc_t* range = new_range();
		if(!range) {
			goto release_and_fail;
		}

		range->ctx = lctx;
		range->addr = virt;
		range->phys = phys;
		range->size = size * PAGE_SIZE;
		range->flags = mflags[i];
		range->next = lctx->ranges;

		if(lctx->ranges) {
			lctx->ranges->previous = range;
		}
		lctx->ranges = range;
		spinlock_release(&lctx->lock);

		if(mvmem[i]) {
			memcpy(mvmem[i], range, sizeof(vm_alloc_t));
		}
	}

	return virt;

release_and_fail:
	for(int i = 0; i < num; i++) {
		spinlock_release(&mctx[i]->lock);
	}

	return NULL;
}

/* Transparently maps memory from one paging context into another.
 */
void* vm_map(struct vm_ctx* ctx, vm_alloc_t* vmem, struct vm_ctx* src_ctx,
	void* src_addr, size_t size, int flags) {

	debug("vm_map: ctx %#x src %p size %#x\n", ctx, src_addr, size);
	void* src_aligned = ALIGN_DOWN(src_addr, PAGE_SIZE);
	size_t src_offset = (uintptr_t)src_addr % PAGE_SIZE;

	/* Get number of pages we have to allocate in the new virtual memory
	 * context. This can be larger than the requested size divided by the page
	 * size when src_addr is not page aligned - For example, when copying 0x100
	 * bytes from 0x1ff0, we need to allocate two pages to map both 0x1000 and
	 * 0x2000 for the full data, even though 0x100 < PAGE_SIZE.
	 */

	if(!spinlock_get(&ctx->lock, -1) || !spinlock_get(&src_ctx->lock, -1)) {
		return NULL;
	}

	size_t size_pages = RDIV(size + src_offset, PAGE_SIZE);
	void* virt = alloc_virt(ctx, size_pages, NULL);
	if(!virt) {
		spinlock_release(&ctx->lock);
		spinlock_release(&src_ctx->lock);
		return NULL;
	}

	debug("  vm_map: allocated %d pages at %p as target\n", size_pages, virt);

	vm_alloc_t* range = new_range();
	if(!range) {
		return NULL;
	}

	range->ctx = ctx;
	range->addr = virt;
	range->size = size_pages * PAGE_SIZE;
	range->flags = flags;
	range->next = ctx->ranges;

	if(ctx->ranges) {
		ctx->ranges->previous = range;
	}
	ctx->ranges = range;

	spinlock_release(&ctx->lock);
	spinlock_release(&src_ctx->lock);

	// Now go over the source ranges in passes and map as much as possible from each range
	// FIXME Currently only maps one page at a time
	//size_t range_offset = src_offset;
	size_t pages_offset = 0;
	int pages_mapped = 0;

	do {
		debug("  vm_map: map pass %d for %p\n", pages_mapped, src_aligned + pages_offset);
		vm_alloc_t* src_range = get_range(src_ctx, src_aligned + pages_offset, false);
		if(!src_range) {
			// FIXME Temp to map around broken execve
			if(flags & VM_MAP_UNDERALLOC_WORKAROUND) {
				pages_mapped++;
				break;
			}

			debug("No range!\n");
			return NULL;
		}


		if(!src_range->phys) {
			panic("vm: Attempt to vm_map sharded memory\n");
		}

		if(flags & VM_MAP_USER_ONLY && !(src_range->flags & VM_USER)) {
			return NULL;
		}

		// See how much we can map from this range
		//size_t range_offset = (uintptr_t)src_addr % PAGE_SIZE;

		//size_t to_copy = MIN(size - mapped, src_range->size - (range_offset % PAGE_SIZE));

		struct vm_alloc_shard* shard = kmalloc(sizeof(struct vm_alloc_shard));
		shard->addr = virt + pages_offset;
		shard->phys = src_range->phys + ALIGN_DOWN(src_addr - src_range->addr, PAGE_SIZE) + pages_offset;
		shard->next = range->shards;
		range->shards = shard;
		debug("vm_mapped %p -> %p\n", shard->addr, shard->phys);

		paging_set_range(ctx->page_dir, shard->addr, shard->phys, PAGE_SIZE, flags);

		pages_offset += PAGE_SIZE;
		pages_mapped++;
	} while(pages_mapped < size_pages);

	assert(pages_mapped == size_pages);

	if(vmem) {
		memcpy(vmem, range, sizeof(vm_alloc_t));
	}

	debug("\n");
	return virt + src_offset;
}

int vm_copy(struct vm_ctx* dest, vm_alloc_t* vmem_dest, vm_alloc_t* vmem_src) {
	// does not work on sharded memory yet
	assert(!vmem_src->shards);

	vm_alloc_t new_kernel_vmem;
	if(!vm_alloc(VM_KERNEL, &new_kernel_vmem, RDIV(vmem_src->size, PAGE_SIZE), NULL, VM_RW)) {
		return -1;
	}

	vm_alloc_t old_kernel_vmem;
	void* old_kernel_virt = vm_map(VM_KERNEL, &old_kernel_vmem, vmem_src->ctx, vmem_src->addr, vmem_src->size, 0);
	if(!old_kernel_virt) {
		return -1;
	}

	memcpy(new_kernel_vmem.addr, old_kernel_virt, vmem_src->size);
	vm_free(&old_kernel_vmem);

	// Zero out remainder of page if needed in order to not leak any previous data
	size_t mod = vmem_src->size % PAGE_SIZE;
	if(mod) {
		void* last_page = new_kernel_vmem.addr + vmem_src->size - PAGE_SIZE;
		bzero(last_page + mod, PAGE_SIZE - mod);
	}

	vm_free(&new_kernel_vmem);
	if(!vm_alloc_at(dest, vmem_dest, RDIV(vmem_src->size, PAGE_SIZE), vmem_src->addr, new_kernel_vmem.phys, vmem_src->flags)) {
		return -1;
	}

	return 0;

	#if 0
	if(!range->ref_count) {
		range->ref_count = kmalloc(sizeof(uint16_t));
		*range->ref_count = 1;
	}

	int flags = range->flags;
	if(range->flags & VM_RW) {
		range->flags |= VM_COW;
		flags |= VM_COW;
	}

	struct vmem_range* new_range = vmem _map(task->vmem_ctx, range->virt_addr, range->phys_addr, range->size, flags);
	vm_alloc_at(&task->vmem, NULL, RDIV(range->size, PAGE_SIZE), range->virt_addr, range->phys_addr, flags);
	__sync_add_and_fetch(range->ref_count, 1);
	new_range->ref_count = range->ref_count;
	#endif
}

int vm_clone(struct vm_ctx* dest, struct vm_ctx* src) {
	vm_alloc_t* range = src->ranges;
	for(; range; range = range->next) {
		if(!(range->flags & VM_TFORK)) {
			continue;
		}

		if(vm_copy(dest, NULL, range) != 0) {
			return -1;
		}
	}

	return 0;
}

int vm_free(vm_alloc_t* range) {
	struct vm_ctx* ctx = range->ctx;
	spinlock_t* lock = &ctx->lock;
	if(!spinlock_get(lock, -1)) {
		return -1;
	}

	// The range passed in is likely an out-of-date copy of the original, so
	// use the self pointer to get current stored version
	range = range->self;

	if(ctx->ranges == range->self) {
		ctx->ranges = range->next;
	}

	if(range->next) {
		range->next->previous = range->previous;
	}

	if(range->previous) {
		range->previous->next = range->next;
	}

	bitmap_clear(&ctx->bitmap, (uintptr_t)range->addr / PAGE_SIZE, RDIV(range->size, PAGE_SIZE));
	spinlock_release(lock);

	paging_clear_range(ctx->page_dir, range->addr, range->size);

	// FIXME VM_FREE should be the default
	if(range->phys && range->flags & VM_FREE) {
		pfree((uintptr_t)range->phys / PAGE_SIZE, RDIV(range->size, PAGE_SIZE));
	}

	struct vm_alloc_shard* shard = range->shards;
	while(shard) {
		struct vm_alloc_shard* old = shard;
		if(range->flags & VM_FREE) {
			pfree((uintptr_t)shard->phys / PAGE_SIZE, RDIV(shard->size, PAGE_SIZE));
		}

		shard = old->next;
		kfree(old);
	}

	kfree(range->self);
	return 0;
}

int vm_new(struct vm_ctx* ctx, struct paging_context* page_dir) {
	ctx->lock = 0;
	ctx->ranges = NULL;
	ctx->bitmap.data = ctx->bitmap_data;
	ctx->bitmap.size = PAGE_ALLOC_BITMAP_SIZE;
	bitmap_clear_all(&ctx->bitmap);

	// Don't allocate null pointer
	bitmap_set(&ctx->bitmap, 0, 1);

	ctx->page_dir = page_dir;
	ctx->page_dir_phys = page_dir;

	// Block NULL page
	bitmap_set(&ctx->bitmap, 0, 1);
	return 0;
}

void vm_cleanup(struct vm_ctx* ctx) {
	if(ctx->page_dir) {
		paging_rm_context(ctx->page_dir);
	}

	vm_alloc_t* range = ctx->ranges;
	while(range) {
		if(range->flags & VM_FREE) {
			pfree((uintptr_t)range->phys / PAGE_SIZE, RDIV(range->size, PAGE_SIZE));
		}

		vm_alloc_t* old_range = range;
		range = range->next;
		kfree(old_range);
	}
}

void* vm_pagedir(struct vm_ctx* ctx) {
	if(!ctx->page_dir) {
		vm_alloc_t vmem;
		if(!vm_alloc(VM_KERNEL, &vmem, 1, NULL, VM_RW | VM_ZERO)) {
			return NULL;
		}

		ctx->page_dir = vmem.addr;
		ctx->page_dir_phys = vmem.phys;

		vm_alloc_t* range = ctx->ranges;

		for(; range; range = range->next) {
			paging_set_range(ctx->page_dir, range->addr, range->phys, range->size, range->flags);
		}
	}
	return ctx->page_dir_phys;
}

int vm_stats(struct vm_ctx* ctx, uint32_t* total, uint32_t* used) {
	*total = ctx->bitmap.size * PAGE_SIZE;
	*used = bitmap_count(&ctx->bitmap) * PAGE_SIZE;
	return 0;
}
