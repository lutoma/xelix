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
#include <mem/vmem.h>
#include <mem/paging.h>
#include <mem/page_alloc.h>
#include <boot/multiboot.h>
#include <fs/sysfs.h>

struct mem_page_alloc_ctx mem_phys_alloc_ctx;

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	uint32_t kmalloc_total, kmalloc_used;
	uint32_t palloc_total, palloc_used;
	kmalloc_get_stats(&kmalloc_total, &kmalloc_used);
	palloc_get_stats(&palloc_total, &palloc_used);

	size_t rsize = 0;
	sysfs_printf("mem_total: %u\n", palloc_total);
	sysfs_printf("mem_used: %u\n", palloc_used - kmalloc_total + kmalloc_used);
	sysfs_printf("mem_shared: %u\n", 0);
	sysfs_printf("mem_cache: %u\n", 0);
	sysfs_printf("palloc_total: %u\n", palloc_total);
	sysfs_printf("palloc_used: %u\n", palloc_used);
	sysfs_printf("kmalloc_total: %u\n", kmalloc_total);
	sysfs_printf("kmalloc_used: %u\n", kmalloc_used);
	return rsize;
}


static void init_phys_allocator() {
	if(mem_page_alloc_new(&mem_phys_alloc_ctx) < 0) {
		panic("palloc: Page allocator initialization failed.\n");
	}

	struct multiboot_tag_mmap* mmap = multiboot_get_mmap();
	struct multiboot_tag_basic_meminfo* mem = multiboot_get_meminfo();
	if(!mmap) {
		panic("palloc_init: Could not get memory maps from multiboot\n");
	}

	log(LOG_INFO, "palloc: Hardware memory map:\n");
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

		log(LOG_INFO, "  %#-12llx - %#-12llx size %#-12llx      %-9s\n",
			entry->addr, entry->addr + entry->len - 1, entry->len, type_names[entry->type]);

		if(entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
			mem_page_alloc_at(&mem_phys_alloc_ctx, (void*)(uint32_t)entry->addr, entry->len / PAGE_SIZE);
		}
	}

	// Leave lower memory and kernel alone
	mem_page_alloc_at(&mem_phys_alloc_ctx, 0, (uintptr_t)ALIGN(KERNEL_END, PAGE_SIZE) / PAGE_SIZE);
	log(LOG_INFO, "palloc: Kernel resides at %#x - %#x\n", KERNEL_START, ALIGN(KERNEL_END, PAGE_SIZE));

	// FIXME mem_info only provides memory size up until first memory hole (~3ish gb)
	uint32_t mem_kb = (MAX(1024, mem->mem_lower) + mem->mem_upper);
	mem_phys_alloc_ctx.bitmap.size = (mem_kb * 1024) / PAGE_SIZE;

	uint32_t used = bitmap_count(&mem_phys_alloc_ctx.bitmap);
	log(LOG_INFO, "palloc: Ready, %u mb, %u pages, %u used, %u free\n",
		mem_kb /  1024, mem_phys_alloc_ctx.bitmap.size, used, mem_phys_alloc_ctx.bitmap.size - used);
}


void mem_init() {
	init_phys_allocator();
	kmalloc_init();
	vmem_init();

	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("mem_info", &sfs_cb);
}
