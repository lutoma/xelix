/* kmalloc.c: Kernel heap
 * Copyright © 2011 Lukas Martini
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

// Thanks to Fritz Grimpen who wrote our original kmalloc.

#include "kmalloc.h"

#include <lib/log.h>
#include <lib/multiboot.h>

static uint32 memoryPosition;
#define pagingEnabled false // FIXME

// For internal use (= In this file) only, see below.
static uint32 __kmalloc(size_t sz, bool align, uint32 *phys)
{
	// If the address is not already page-aligned
	if (align == 1 && (memoryPosition & 0xFFFFF000))
	{
		// Align it.
		memoryPosition &= 0xFFFFF000;
		memoryPosition += 0x1000;
	}

	if (phys)
		*phys = memoryPosition;

	uint32 tmp = memoryPosition;
	memoryPosition += sz;
	return tmp;
}

void kfree(void *ptr)
{
	return;
}

/* A few shortcuts so one doesn't always have to pass all the
 * parameters all the time.
 */

// vanilla
uint32 kmalloc(size_t sz)
{
	return __kmalloc(sz, false, NULL);
}

// page aligned.
uint32 kmalloc_a(size_t sz)
{
	return __kmalloc(sz, true, NULL);
}

// returns a physical address.
uint32 kmalloc_p(size_t sz, uint32 *phys)
{
	return __kmalloc(sz, false, phys);
}

// page aligned and returns a physical address.
uint32 kmalloc_ap(size_t sz, uint32 *phys)
{
	return __kmalloc(sz, true, phys);
}

void kmalloc_init()
{
	/* The modules are the last things the bootloader loads after the
	 * kernel. Therefore, we can securely assume that everything after
	 * the last module's end should be free.
	 */
	memoryPosition = multiboot_info->modsAddr[multiboot_info->modsCount - 1].end;
}
