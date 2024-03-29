/* mem.c: Memory management
 * Copyright © 2020-2021 Lukas Martini
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


#include <string.h>
#include <bitmap.h>
#include <panic.h>
#include <spinlock.h>
#include <mem/mem.h>
#include <mem/kmalloc.h>
#include <mem/paging.h>
#include <mem/page_alloc.h>
#include <mem/vm.h>
#include <boot/multiboot.h>
#include <fs/sysfs.h>

struct mem_page_alloc_ctx mem_phys_alloc_ctx;
struct vm_ctx vm_kernel_ctx;

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	uint32_t kmalloc_total, kmalloc_used;
	uint32_t palloc_total, palloc_used;
	uint32_t vm_total, vm_used;

	kmalloc_get_stats(&kmalloc_total, &kmalloc_used);
	mem_page_alloc_stats(&mem_phys_alloc_ctx, &palloc_total, &palloc_used);
	vm_stats(&vm_kernel_ctx, &vm_total, &vm_used);

	size_t rsize = 0;
	sysfs_printf("mem_total: %u\n", palloc_total);
	sysfs_printf("mem_used: %u\n", palloc_used - kmalloc_total + kmalloc_used);
	sysfs_printf("mem_shared: %u\n", 0);
	sysfs_printf("mem_cache: %u\n", 0);
	sysfs_printf("palloc_total: %u\n", palloc_total);
	sysfs_printf("palloc_used: %u\n", palloc_used);
	sysfs_printf("vm_total: %u\n", vm_total);
	sysfs_printf("vm_used: %u\n", vm_used);
	sysfs_printf("kmalloc_total: %u\n", kmalloc_total);
	sysfs_printf("kmalloc_used: %u\n", kmalloc_used);
	return rsize;
}

void mem_init(void) {
	// Init phys page allocator. kernel vm has already been initialized in i386-paging.c.
	if(mem_page_alloc_new(&mem_phys_alloc_ctx) < 0) {
		panic("mem: Initialization of phys page allocator failed.\n");
	}

	// Fetch memory information from multiboot
	struct multiboot_tag_mmap* mmap = multiboot_get_mmap();
	struct multiboot_tag_basic_meminfo* mem = multiboot_get_meminfo();
	if(!mmap) {
		panic("mem: Could not get memory maps from multiboot\n");
	}

	// Block all regions not marked as available in the physical page allocator
	log(LOG_INFO, "mem: Hardware memory map:\n");
	uint32_t offset = 16;
	for(; offset < mmap->size; offset += mmap->entry_size) {
		struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)((intptr_t)mmap + offset);

		const char* type_names[] = {
			"Unknown",
			"Available",
			"Reserved",
			"ACPI",
			"NVS",
			"Bad"
		};

		log(LOG_INFO, "  %#010llx - %#010llx size %#-10llx      %-9s\n",
			entry->addr, entry->addr + entry->len - 1, entry->len, type_names[entry->type]);

		if(entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
			mem_page_alloc_at(&mem_phys_alloc_ctx, (void*)(uint32_t)entry->addr, entry->len / PAGE_SIZE);
		}
	}

	log(LOG_INFO, "mem: Kernel resides at %p - %p\n", KERNEL_START, ALIGN(KERNEL_END, PAGE_SIZE));

	// FIXME mem_info only provides memory size up until first memory hole (~3ish gb)
	uint32_t mem_kb = (MAX(1024, mem->mem_lower) + mem->mem_upper);
	mem_phys_alloc_ctx.bitmap.size = (mem_kb * 1024) / PAGE_SIZE;

	uint32_t pused = bitmap_count(&mem_phys_alloc_ctx.bitmap);
	log(LOG_INFO, "mem: Phys page allocator ready, %u mb, %u pages, %u used, %u free\n",
		mem_kb /  1024, mem_phys_alloc_ctx.bitmap.size, pused, mem_phys_alloc_ctx.bitmap.size - pused);

	uint32_t vused = bitmap_count(&vm_kernel_ctx.bitmap);
	log(LOG_INFO, "mem: Virt page allocator ready, %u pages, %u used, %u free\n",
		vm_kernel_ctx.bitmap.size, vused, vm_kernel_ctx.bitmap.size - vused);

}

void mem_late_init(void) {
	/* In physical memory, block out all lower memory up to the end of early
	 * allocations from paging.c. Since the early allocations follow
	 * KERNEL_END, this implicitly includes the kernel binary.
	 */
	mem_page_alloc_at(&mem_phys_alloc_ctx, 0, (uintptr_t)paging_alloc_end / PAGE_SIZE);

	kmalloc_init();

	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("mem_info", &sfs_cb);
}
