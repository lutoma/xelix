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
#include "vm.h"
#include <lib/log.h>
#include <arch/i386/lib/multiboot.h>

uint32_t memoryPosition;
#define pagingEnabled false // FIXME

/* Use the macros instead of directly calling this functions.
 * For details on the __attribute__((alloc_size(1))), see the GCC
 * documentation at http://is.gd/6gmEqk.
 */
void* __attribute__((alloc_size(1))) __kmalloc(size_t sz, bool align, uint32_t *phys)
{
	// If the address is not already page-aligned
	if (align == 1 && (memoryPosition & 0xFFFFF000))
		memoryPosition = VM_ALIGN(memoryPosition);

	if (phys)
		*phys = memoryPosition;

	uint32_t tmp = memoryPosition;
	memoryPosition += sz;
	return (void*)tmp;
}

void kfree(void *ptr)
{
	return;
}


uint32_t kmalloc_getMemoryPosition()
{
	return memoryPosition;
}

void kmalloc_init()
{
	/* The modules are the last things the bootloader loads after the
	 * kernel. Therefore, we can securely assume that everything after
	 * the last module's end should be free.
	 */
	if(multiboot_info->modsCount > 0) // Do we have at least one module?
		memoryPosition = multiboot_info->modsAddr[multiboot_info->modsCount - 1].end;
	else // Guess.
		memoryPosition = 15 * 1024 * 1024;

}
