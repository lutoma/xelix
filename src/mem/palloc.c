/* palloc.c: Page allocator
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

#include "palloc.h"
#include <mem/paging.h>
#include <boot/multiboot.h>
#include <string.h>
#include <bitmap.h>
#include <panic.h>

#define BITMAP_SIZE 0xfffff000 / PAGE_SIZE
static uint32_t pages_bitmap_data[bitmap_size(BITMAP_SIZE)];
static struct bitmap pages_bitmap = {
	.data = pages_bitmap_data,
	.size = BITMAP_SIZE,
	.first_free = 0,
};

void* palloc(uint32_t size) {
	uint32_t num = bitmap_find(&pages_bitmap, size);
	bitmap_set(&pages_bitmap, num, size);
	return (void*)(num * PAGE_SIZE);
}

void pfree(uint32_t num, uint32_t size) {
	bitmap_clear(&pages_bitmap, num, size);
}

void palloc_init() {
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
			bitmap_set(&pages_bitmap, (uint32_t)entry->addr / PAGE_SIZE, entry->len / PAGE_SIZE);
		}
	}

	// Leave lower memory and kernel alone
	bitmap_set(&pages_bitmap, 0, (uintptr_t)ALIGN(KERNEL_END, PAGE_SIZE) / PAGE_SIZE);
	log(LOG_INFO, "palloc: Kernel resides at %#x - %#x\n", KERNEL_START, ALIGN(KERNEL_END, PAGE_SIZE));

	// FIXME mem_info only provides memory size up until first memory hole (~3ish gb)
	uint32_t mem_kb = (MAX(1024, mem->mem_lower) + mem->mem_upper);
	pages_bitmap.size = (mem_kb * 1024) / PAGE_SIZE;

	uint32_t used = bitmap_count(&pages_bitmap);
	log(LOG_INFO, "palloc: Ready, %u mb, %u pages, %u used, %u free\n",
		mem_kb /  1024, pages_bitmap.size, used, pages_bitmap.size - used);
}

void palloc_get_stats(uint32_t* total, uint32_t* used) {
	*total = pages_bitmap.size * PAGE_SIZE;
	*used = bitmap_count(&pages_bitmap) * PAGE_SIZE;
}
