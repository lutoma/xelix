/* kmalloc.c: Kernel memory allocator
 * Copyright © 2016-2018 Lukas Martini
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

#include "kmalloc.h"
#include "track.h"
#include "vmem.h"
#include <lib/log.h>
#include <lib/multiboot.h>
#include <lib/panic.h>
#include <lib/spinlock.h>

#define KMALLOC_MAGIC 0xCAFE

/* Enable debugging. This will send out cryptic debug codes to the serial line
 * during kmalloc()/free()'s. Also makes everything horribly slow. */
#ifdef KMALLOC_DEBUG
	#define SERIAL_DEBUG(args...) { if(vmem_kernelContext) serial_printf(args); }
#else
	#define SERIAL_DEBUG(args...)
#endif

#define PREV_FOOTER(x) ((uint32_t*)((intptr_t)x - sizeof(uint32_t)))
#define GET_FOOTER(x) ((uint32_t*)((intptr_t)x + x->size + sizeof(struct mem_block)))
#define PREV_BLOCK(x) ((struct mem_block*)((intptr_t)PREV_FOOTER(x) \
	- (*PREV_FOOTER(x)) - sizeof(struct mem_block)))
#define NEXT_BLOCK(x) ((struct mem_block*)((intptr_t)GET_FOOTER(x) + sizeof(uint32_t)))

struct mem_block {
	uint16_t magic;
	uint32_t size;

	enum {
		TYPE_KERNEL,
		TYPE_TASK,
		TYPE_FREE
	} type;

	// TYPE_TASK: Process ID
	// TYPE_FREE: Pointer to next free block, if any
	// TYPE_KERNEL: Unused
	union {
		uint32_t pid;
		struct mem_block* next_free;
	};
};

static bool initialized = false;
static spinlock_t kmalloc_lock;
struct mem_block* last_free = (struct mem_block*)NULL;
static intptr_t alloc_start;
static intptr_t alloc_end;
static intptr_t alloc_max;


static struct mem_block* find_free_block(size_t sz, bool align) {
	if(align) return NULL; // DEBUG
	SERIAL_DEBUG("FFB ");

	struct mem_block* prev = NULL;
	for(struct mem_block* fblock = last_free; fblock; fblock = fblock->next_free) {
		if(unlikely(fblock->magic != KMALLOC_MAGIC || fblock->type != TYPE_FREE)) {
			panic("find_free_block: Metadata corruption in free blocks linked list\n");
			return NULL;
		}

		size_t sz_needed = sz;
		intptr_t potential_result = (intptr_t)fblock + sizeof(struct mem_block);

		if(align && (potential_result & (PAGE_SIZE - 1))) {
			uint32_t offset = VMEM_ALIGN((uint32_t)potential_result + sizeof(struct mem_block)) - (potential_result + sizeof(struct mem_block));

			/* We need at least x bytes to store the headers and footers of our
			 * block and of the new block we'll create in the offset
		 	*/
			if(offset < 0x100) {
				offset += PAGE_SIZE;
			}

			sz_needed += offset;
			SERIAL_DEBUG("New size needed: 0x%x\n");
		}

		if(fblock->size >= sz) {
			SERIAL_DEBUG("HIT 0x%x size 0x%x ", fblock, fblock->size);

			if(prev) {
				prev->next_free = fblock->next_free;
			} else {
				last_free = fblock->next_free;
			}

			return fblock;
		}

		prev = fblock;
	}

	return NULL;
}

static struct mem_block* set_block(size_t sz, intptr_t position) {
	struct mem_block* header = (struct mem_block*)position;

	if(likely((intptr_t)header != alloc_start)) {
		struct mem_block* prev = PREV_BLOCK(header);

		SERIAL_DEBUG("\nChecking header at 0x%x, footer at 0x%x\n", prev, GET_FOOTER(prev));
		if(unlikely(prev->magic != KMALLOC_MAGIC)) {
			panic("set_block: Metadata corruption (previous block with invalid magic)");
		}
	}

	header->size = sz;
	header->magic = KMALLOC_MAGIC;

	// Add uint32_t footer with size so we can find the header
	uint32_t* footer = GET_FOOTER(header);
	SERIAL_DEBUG("Writing header at 0x%x, footer at 0x%x\n", header, footer);
	*footer = header->size;
	return header;
}

/* Use the macros instead of directly calling this function.
 * For details on the attributes, see the GCC documentation at http://is.gd/6gmEqk.
 * FIXME Aligned allocations cannot currently reuse existing free blocks and
 * unnecessarily waste memory.
 */
void* __attribute__((alloc_size(1))) _kmalloc(size_t sz, bool align, uint32_t pid,
	const char* _debug_file, uint32_t _debug_line, const char* _debug_func) {

	if(unlikely(!initialized)) {
		panic("Attempt to kmalloc before allocator is initialized.\n");
	}

	SERIAL_DEBUG("%s:%d %s\nALLC 0x%x ", _debug_file, _debug_line, _debug_func, sz);

	#ifdef KMALLOC_DEBUG
		if(sz >= (1024 * 1024)) {
			SERIAL_DEBUG("(%d MB) ", sz / (1024 * 1024));
		} else if(sz >= 1024) {
			SERIAL_DEBUG("(%d KB) ", sz / 1024);
		}
	#endif

	if(unlikely(!spinlock_get(&kmalloc_lock, 30))) {
		SERIAL_DEBUG("Could not get spinlock\n\n");
		return NULL;
	}

	struct mem_block* header = find_free_block(sz, align);
	bool need_alloc_end = false;

	if(!header) {
		SERIAL_DEBUG("NEW ");

		if(alloc_end + sz >= alloc_max) {
			panic("Out of memory");
		}

		header = set_block(sz, alloc_end);
		alloc_end = (uint32_t)GET_FOOTER(header) + sizeof(uint32_t);

		if(align) {
			need_alloc_end = true;
		}
	}

	intptr_t result = (void*)((uint32_t)header + sizeof(struct mem_block));

	/* When alignment is requested (i.e. result mod PAGE_SIZE should be 0), we
	 * check if the result is already aligned (second cond). If not, move the
	 * block up until it's at an aligned position, and create a new free offset
	 * block in the created space.
	 */
	if(align && (result & (PAGE_SIZE - 1))) {
		SERIAL_DEBUG("ALIGN original result 0x%x\n", result);

		uint32_t header_offset = VMEM_ALIGN(result)
			- result
			- sizeof(struct mem_block);

		/* We need at least x bytes to store the headers and footers of our
		 * block and of the new block we'll create in the offset
		 * FIXME Calculate this properly (Should be much lower)
		 */
		if(header_offset < 0x100) {
			header_offset += PAGE_SIZE;
		}

		SERIAL_DEBUG("New result is at 0x%x\n", result + header_offset + sizeof(struct mem_block));
		SERIAL_DEBUG("Offset header size ox%x\n", header_offset - sizeof(uint32_t));

		/* Switch the settings of the original block to be suitable for the
		 * offset and add to the free blocks list.
		 */
		struct mem_block* offset_header = set_block(header_offset - sizeof(uint32_t), (intptr_t)header);
		offset_header->type = TYPE_FREE;
		offset_header->next_free = last_free;
		last_free = offset_header;


		// Add a new block header for the original allocation
		SERIAL_DEBUG("\n\nSetting result block at 0x%x\n", NEXT_BLOCK(offset_header));
		header = set_block(sz, NEXT_BLOCK(offset_header));

		result = (intptr_t)header + sizeof(struct mem_block);
		SERIAL_DEBUG("\nFinal result: 0x%x\n", result);
	}

	header->type = pid ? TYPE_TASK : TYPE_KERNEL;
	header->pid = pid;

	if(need_alloc_end) {
		alloc_end = (uint32_t)GET_FOOTER(header) + sizeof(uint32_t);
	}

	spinlock_release(&kmalloc_lock);
	SERIAL_DEBUG("RESULT 0x%x\n\n", (intptr_t)result);
	return (void*)result;
}

// FIXME Cannot free aligned blocks right now due to the inconsistent header offset
void _kfree(void *ptr, const char* _debug_file, uint32_t _debug_line, const char* _debug_func)
{
	SERIAL_DEBUG("%s:%d %s\nFREE 0x%x ", _debug_file, _debug_line, _debug_func, ptr);
	struct mem_block* header = (struct mem_block*)((intptr_t)ptr - sizeof(struct mem_block));

	if(unlikely((intptr_t)header < alloc_start || (intptr_t)ptr >= alloc_end)) {
		SERIAL_DEBUG("INVALID_BOUNDS\n\n");
		return;
	}

	if(unlikely(header->magic != KMALLOC_MAGIC)) {
		SERIAL_DEBUG("INVALID_MAGIC\n\n");
		return;
	}

	if(unlikely(!spinlock_get(&kmalloc_lock, 30))) {
		SERIAL_DEBUG("Could not get spinlock\n\n");
		return;
	}

	// Check previous block
	if(likely((intptr_t)header > alloc_start)) {
		struct mem_block* prev = PREV_BLOCK(header);
		SERIAL_DEBUG("PBH 0x%x sz 0x%x ", prev, *PREV_FOOTER(header));

		if(unlikely(prev->magic != KMALLOC_MAGIC)) {
			panic("kfree: Metadata corruption (Previous block with invalid magic)\n");
		}
	}

	// TODO Merge adjacent free blocks
	header->type = TYPE_FREE;
	header->next_free = last_free;
	last_free = header;



	SERIAL_DEBUG("\n\n");
	spinlock_release(&kmalloc_lock);
}

void kmalloc_init()
{
	memory_track_area_t* largest_area = NULL;
	for(int i = 0; i < memory_track_num_areas; i++) {
		memory_track_area_t* area = &memory_track_areas[i];

		if(area->type == MEMORY_TYPE_FREE && (!largest_area || largest_area->size < area->size)) {
			largest_area = area;
		}
	}

	if(!largest_area) {
		panic("kmalloc: Could not find suitable memory area");
	}

	largest_area->type = MEMORY_TYPE_KMALLOC;
	alloc_start = alloc_end = largest_area->addr;
	alloc_max = (intptr_t)largest_area->addr + largest_area->size;
	initialized = true;
}
