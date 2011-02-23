#pragma once

/* Copyright © 2010 Christoph Sünderhauf
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

#include <common/generic.h>

void* kmalloc(size_t numbytes);

/* memory aligned to 4kb.
 * physicalAddress: If physicalAddress is not 0, the physical address of
 * the memory returned is written into that location. The physical
 * address is important when paging is already enabled: Then we can only
 * access memory via its virtual address, but eg. new page directories
 * need to containt physical addresses to their page tables.
 */
void* kmalloc_aligned(size_t numbytes, uint32* physicalAddress);

typedef struct {
	size_t size;
	uint8 free:1;
} memorySection_t;

void kfree(void *ptr);
void kmalloc_init(uint32 start);
