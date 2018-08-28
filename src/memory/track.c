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

#include <memory/track.h>
#include <log.h>
#include <print.h>
#include <multiboot.h>
#include <hw/serial.h>

// Symbols defined by LD in linker.ld
extern void* __kernel_start;
extern void* __kernel_end;

// Set up the memory areas marked as free in the multiboot headers
static void copy_multiboot_areas(uint32_t mmap_addr, uint32_t mmap_length) {
	uint32_t i = mmap_addr;
	while (i < (mmap_addr + mmap_length))
	{
		// FIXME This should use struct multiboot_memory_map.
		uint32_t *size = (uint32_t *) i;
		uint32_t *base_addr_low = (uint32_t *) (i + 4);
		uint32_t *length_low = (uint32_t *) (i + 12);
		uint32_t *type = (uint32_t *) (i + 20);

		memory_track_type_t area_type;
		switch(*type) {
			case 1: area_type = MEMORY_TYPE_FREE; break;
			case 3: area_type = MEMORY_TYPE_ACPI; break;
			case 4: area_type = MEMORY_TYPE_HIBERNATE; break;
			case 5: area_type = MEMORY_TYPE_DEFECTIVE; break;
			default: area_type = MEMORY_TYPE_UNKNOWN;
		}

		memory_track_area_t* area = &memory_track_areas[memory_track_num_areas++];
		area->addr = (void*)*base_addr_low;
		area->size = *length_low;
		area->type = area_type;

		// Check if this block contains the kernel, and if so create a separate area for that
		if(*base_addr_low >= (intptr_t)&__kernel_start && *base_addr_low <= (intptr_t)&__kernel_end) {
			uint32_t kernel_size = (intptr_t)&__kernel_end - (intptr_t)&__kernel_start;

			memory_track_area_t* kernel_area;

			// Check if the kernel starts at the same block as this area. If so, just recycle it,
			// otherwise create a new kernel area and set the end of this one accordingly.
			if(area->addr == &__kernel_start) {
				kernel_area = area;
			} else {
				kernel_area = &memory_track_areas[memory_track_num_areas++];
				area->size = (intptr_t)&__kernel_start - (intptr_t)area->addr;
			}

			// Add kernel area
			kernel_area->addr = (void*)&__kernel_start;
			kernel_area->size = kernel_size;
			kernel_area->type = MEMORY_TYPE_KERNEL_BINARY;

			// Add remainder of the original block (if any)
			if(*base_addr_low + *length_low > (intptr_t)&__kernel_end) {
				memory_track_area_t* remainder_area = &memory_track_areas[memory_track_num_areas++];
				remainder_area->addr = (void*)&__kernel_end;
				remainder_area->size = *length_low - kernel_size;
				remainder_area->type = area_type;
			}

		}

		i += *size + 4;
	}
}

void memory_track_print_areas() {
	log(LOG_DEBUG, "memory_track: Areas:\n");

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
		printf("    #%-2d %-9s at 0x%-9x end 0x%-9x size 0x%-9x\n",
			i, type_names[area->type], area->addr, area->addr + area->size - 1, area->size);
	}
}

void memory_track_init(multiboot_info_t* multiboot_info)
{
	memset(memory_track_areas, 0, sizeof(memory_track_area_t) * MEMORY_TRACK_MAX_AREAS);
	copy_multiboot_areas(multiboot_info->mmap_addr, multiboot_info->mmap_length);

	// Add area for initrd(s)
	for(int i = 0; i < multiboot_info->mods_count; i++) {
		memory_track_area_t* area = &memory_track_areas[memory_track_num_areas++];
		area->addr = (void*)multiboot_info->mods_addr[i].start;
		area->size = multiboot_info->mods_addr[i].end - multiboot_info->mods_addr[i].start;
		area->type = MEMORY_TYPE_INITRD;
	}

	// Zero free areas
	for(int i = 0; i < memory_track_num_areas; i++) {
		memory_track_area_t* area = &memory_track_areas[i];

		if(area->type == MEMORY_TYPE_FREE) {
			memset(area->addr, 0, area->size);
		}
	}
}
