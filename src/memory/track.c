/* track.c: Keep track of physical memory sections and their use
 * Copyright © 2015 Lukas Martini
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

#include <lib/generic.h>
#include <memory/track.h>
#include <lib/log.h>
#include <lib/print.h>
#include <lib/multiboot.h>

// Set up the memory areas marked as free in the multiboot headers
static void copy_multiboot_areas(uint32_t mmap_addr, uint32_t mmap_length) {
	uint32_t i = mmap_addr;
	while (i < (mmap_addr + mmap_length))
	{
		// FIXME This should use the multiboot_memoryMap_t struct.
		uint32_t *size = (uint32_t *) i;
		uint32_t *base_addr_low = (uint32_t *) (i + 4);
		uint32_t *length_low = (uint32_t *) (i + 12);
		uint32_t *type = (uint32_t *) (i + 20);

		memory_track_area_t* area = &memory_track_areas[memory_track_num_areas++];
		area->addr = (void*)*base_addr_low;
		area->size = *length_low;
		area->type = (*type == 1) ? MEMORY_TYPE_FREE : MEMORY_TYPE_UNKNOWN;

		i += *size + 4;
	}
}

void memory_track_print_areas() {
	log(LOG_DEBUG, "memory_track: Areas:\n");

	for(int i = 0; i < memory_track_num_areas; i++) {
		memory_track_area_t* area = &memory_track_areas[i];
		printf("\tArea #%d at 0x%x, size %d, type %d\n", i, area->addr, area->size, area->type);
	}
}

void memory_track_init(multiboot_info_t* multiboot_info)
{
	memset(memory_track_areas, 0, sizeof(memory_track_area_t) * MEMORY_TRACK_MAX_AREAS);
	copy_multiboot_areas(multiboot_info->mmapAddr, multiboot_info->mmapLength);

	// Add area for initrd(s)
	for(int i = 0; i < multiboot_info->modsCount; i++) {
		memory_track_area_t* area = &memory_track_areas[memory_track_num_areas++];
		area->addr = (void*)multiboot_info->modsAddr[i].start;
		area->size = multiboot_info->modsAddr[i].end - multiboot_info->modsAddr[i].start;
		area->type = MEMORY_TYPE_INITRD;
	}
}