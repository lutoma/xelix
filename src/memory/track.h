#pragma once

/* Copyright © 2015 Lukas Martini
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
#include <lib/multiboot.h>

#define MEMORY_TRACK_MAX_AREAS 256

typedef enum {
	MEMORY_TYPE_FREE,
	MEMORY_TYPE_KERNEL_BINARY,
	MEMORY_TYPE_INITRD,
	MEMORY_TYPE_ACPI,
	MEMORY_TYPE_HIBERNATE,
	MEMORY_TYPE_DEFECTIVE,
	MEMORY_TYPE_KMALLOC,
	MEMORY_TYPE_UNKNOWN
} memory_track_type_t;

typedef struct {
	void* addr;
	size_t size;
	memory_track_type_t type;
} memory_track_area_t;

memory_track_area_t memory_track_areas[MEMORY_TRACK_MAX_AREAS];
uint32_t memory_track_num_areas;

void memory_track_print_areas();
void memory_track_init(multiboot_info_t* multiboot_info);
