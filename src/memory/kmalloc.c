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
#include "vmem.h"
#include "track.h"
#include <lib/log.h>
#include <lib/panic.h>
#include <hw/serial.h>
#include <lib/multiboot.h>

static uint32_t memoryPosition;
static bool initialized = false;

/* Use the macros instead of directly calling this functions.
 * For details on the __attribute__((alloc_size(1))), see the GCC
 * documentation at http://is.gd/6gmEqk.
 */
void* __attribute__((alloc_size(1))) __kmalloc(size_t sz, bool align, uint32_t *phys, const char* _debug_file, uint32_t _debug_line, const char* _debug_func)
{
	if(unlikely(!initialized)) {
		serial_print("Call to kmalloc before it's initialized.\n");
		serial_print(_debug_file);
		serial_print("\n");
		serial_print(_debug_func);
		panic("Call to kmalloc before it's initialized.");
	}

	// If the address is not already page-aligned
	if (align == 1 && (memoryPosition & 0xFFFFF000))
		memoryPosition = VMEM_ALIGN(memoryPosition);

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
