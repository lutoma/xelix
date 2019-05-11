/* track.c: Keep track of physical memory sections and their use
 * Copyright © 2015-2018 Lukas Martini
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

#include <mem/track.h>
#include <log.h>
#include <print.h>
#include <panic.h>
#include <string.h>
#include <multiboot.h>
#include <hw/serial.h>

#ifdef __i386__
// Set up the memory areas marked as free in the multiboot headers
static void copy_multiboot_areas() {
	struct multiboot_tag_mmap* mmap = multiboot_get_mmap();
	if(!mmap) {
		panic("Could not get memory map from multiboot info\n");
	}

	uint32_t offset = 16;
	for(; offset < mmap->size; offset += mmap->entry_size) {
		struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)((intptr_t)mmap + offset);

		memory_track_type_t area_type;
		switch(entry->type) {
			case 1: area_type = MEMORY_TYPE_FREE; break;
			case 2: area_type = MEMORY_TYPE_UNKNOWN; break;
			case 3: area_type = MEMORY_TYPE_ACPI; break;
			case 4: area_type = MEMORY_TYPE_HIBERNATE; break;
			case 5: area_type = MEMORY_TYPE_DEFECTIVE; break;
			default: area_type = MEMORY_TYPE_UNKNOWN;
		}

		memory_track_area_t* area = &memory_track_areas[memory_track_num_areas++];
		area->addr = (void*)(uint32_t)entry->addr;
		area->size = entry->len;
		area->type = area_type;

		// Check if this block contains the kernel, and if so create a separate area for that
		if(entry->addr >= (intptr_t)KERNEL_START && entry->addr <= (intptr_t)KERNEL_END) {
			memory_track_area_t* kernel_area;

			// Check if the kernel starts at the same block as this area. If so, just recycle it,
			// otherwise create a new kernel area and set the end of this one accordingly.
			if(area->addr == KERNEL_START) {
				kernel_area = area;
			} else {
				kernel_area = &memory_track_areas[memory_track_num_areas++];
				area->size = (intptr_t)KERNEL_START - (intptr_t)area->addr;
			}

			// Add kernel area
			kernel_area->addr = KERNEL_START;
			kernel_area->size = KERNEL_SIZE;
			kernel_area->type = MEMORY_TYPE_KERNEL_BINARY;

			// Add remainder of the original block (if any)
			if(entry->addr + entry->len > (intptr_t)KERNEL_END) {
				memory_track_area_t* remainder_area = &memory_track_areas[memory_track_num_areas++];
				remainder_area->addr = KERNEL_END;
				remainder_area->size = entry->len - KERNEL_SIZE;
				remainder_area->type = area_type;
			}

		}
	}
}
#endif

void memory_track_init() {
	memory_track_num_areas = 0;

	#ifdef __arm__
		memory_track_area_t* area = &memory_track_areas[memory_track_num_areas++];
		area->addr = (void*)0x300000;
		area->size = 0x1000000;
		area->type = MEMORY_TYPE_FREE;
	#else
		copy_multiboot_areas();
	#endif

	log(LOG_INFO, "memory_track: Areas:\n");

	char* type_names[] = {
		"Free",
		"Kernel",
		"Initrd",
		"ACPI",
		"Hibernate",
		"Defective",
		"kmalloc",
		"Unknown"
	};

	for(int i = 0; i < memory_track_num_areas; i++) {
		memory_track_area_t* area = &memory_track_areas[i];
		log(LOG_INFO, "  #%-2d %-9s at 0x%-9x end 0x%-9x size 0x%-9x\n",
			i, type_names[area->type], area->addr, area->addr + area->size - 1, area->size);
	}
}
