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
#include "vmem.h"
#include <lib/log.h>
#include <lib/multiboot.h>
#include <lib/panic.h>
#include <lib/spinlock.h>

/* Enable debugging. This will send out cryptic debug codes to the serial line
 * during kmalloc()/free()'s. Also makes everything horribly slow. */
#ifdef KMALLOC_DEBUG
	#include <hw/serial.h>
	#define SERIAL_DEBUG serial_print
#else
	#define SERIAL_DEBUG
#endif

#define KMALLOC_MAGIC 0xCAFE
#define PREV_FOOTER(x) (*(uint32_t*)((uint32_t)x - sizeof(uint32_t)))
#define PREV_BLOCK(x) ((struct mem_block*)(x - sizeof(uint32_t) - sizeof(struct mem_block) - PREV_FOOTER(x)))
#define NEXT_BLOCK(x) ((struct mem_block*)((uint32_t)x + sizeof(struct mem_block) + x->size + sizeof(uint32_t)))
#define GET_FOOTER(x) ((uint32_t)((uint32_t)x + x->size + sizeof(struct mem_block)))

static bool initialized = false;
static uint32_t alloc_start;
static uint32_t alloc_end;
static uint32_t num_blocks = 0;
static uint32_t total_blocks_size = 0;

static spinlock_t kmalloc_lock;

static struct mem_block {
	uint16_t magic;
	bool status:1;
	uint32_t size;
	uint8_t type;
	uint32_t pid;
};

/* TODO Automatically increase the size of this and make the initial smaller
 * It's only ~400K right now anyway though. */
#define MAX_FREE_BLOCKS 100000
static struct mem_block* free_blocks[MAX_FREE_BLOCKS];
static uint32_t next_free_block_pos = 0;
static uint32_t last_free_block_pos = 0;

/* Use the macros instead of directly calling this function.
 * For details on the attributes, see the GCC documentation at http://is.gd/6gmEqk.
 * FIXME Aligned allocations cannot currently reuse existing free blocks and
 * unnecessarily waste memory.
 */
void* __attribute__((alloc_size(1))) __kmalloc(size_t sz, bool align, uint32_t *phys, const char* _debug_file, uint32_t _debug_line, const char* _debug_func)
{
	if(unlikely(!initialized)) {
		panic("Attempt to kmalloc() before allocator is initialized.\n");
		return NULL;
	}

	SERIAL_DEBUG(_debug_file);
	SERIAL_DEBUG(": ");
	SERIAL_DEBUG(_debug_func);
	SERIAL_DEBUG("\nALLC ");

	#ifdef KMALLOC_DEBUG
		char itoa_result[100];
		itoa(sz, itoa_result, 16);
		serial_print("0x");
		serial_print(itoa_result);
		serial_print(" ");

		if(sz >= 1024) {
			serial_print("(");
			itoa(sz/1024, itoa_result, 10);
			serial_print(itoa_result);
			serial_print(" kB) ");
		}
	#endif

	if(unlikely(!spinlock_get(&kmalloc_lock, 30))) {
		SERIAL_DEBUG("Could not get spinlock\n");
		return NULL;
	}

	struct mem_block* header = NULL;

	// If there are any blocks: Find suitable free block
	if(likely(alloc_end > alloc_start && !align)) {
		SERIAL_DEBUG("SFB ");

		for(int i = 0; i <= last_free_block_pos; i++) {
			struct mem_block* fblock = free_blocks[i];

			if(fblock && fblock->magic == KMALLOC_MAGIC && fblock->size >= sz) {
				SERIAL_DEBUG("HIT ");

				header = fblock;
				fblock = NULL;
				break;
			}
		}
	}

	// As last resort, just add a new block
	// FIXME Does the second cond make sense?
	if(!header || header->magic != KMALLOC_MAGIC) {
		SERIAL_DEBUG("NEW ");

		#ifdef KMALLOC_DEBUG
			itoa((intptr_t)alloc_end, itoa_result, 16);
			serial_print("0x");
			serial_print(itoa_result);
			serial_print(" ->");
		#endif

		header = (struct mem_block*)alloc_end;
		header->size = sz;
		header->magic = KMALLOC_MAGIC;
		alloc_end = (uint32_t)GET_FOOTER(header) + sizeof(uint32_t);

		#ifdef KMALLOC_DEBUG
			itoa((intptr_t)alloc_end, itoa_result, 16);
			serial_print(" 0x");
			serial_print(itoa_result);
			serial_print(" ");
		#endif

		total_blocks_size += sz + sizeof(uint32_t) + sizeof(struct mem_block);
		num_blocks++;
	}

	// Mark block as our own
	header->status = true;

	// FIXME
	header->type = 42;
	header->pid = 0;

	void* result;
	// If the address is not already page-aligned
	if(unlikely(align && (alloc_end & 0xFFFFF000))) {
		SERIAL_DEBUG("ALIGN ");
		uint32_t old_pos = (uint32_t)header + sizeof(struct mem_block);
		result = VMEM_ALIGN((uint32_t)header + sizeof(struct mem_block));
		header->size += result - old_pos;

		// FIXME
		alloc_end += result - old_pos;
	} else {
		result = (void*)((uint32_t)header + sizeof(struct mem_block));
	}

	// Add footer with allocation size for reverse lookups
	uint32_t* footer = GET_FOOTER(header);
	*footer = sz;

	spinlock_release(&kmalloc_lock);

	// FIXME Remove this
	if(unlikely(phys))
		*phys = (void*)((uint32_t)header + sizeof(struct mem_block));

	#ifdef KMALLOC_DEBUG
		itoa(((uint32_t)header + sizeof(struct mem_block)), itoa_result, 16);
		serial_print("RESULT 0x");
		serial_print(itoa_result);
		serial_print("\n");
	#endif

	return result;
}

void __kfree(void *ptr, const char* _debug_file, uint32_t _debug_line, const char* _debug_func)
{

	SERIAL_DEBUG(_debug_file);
	SERIAL_DEBUG(": ");
	SERIAL_DEBUG(_debug_func);
	SERIAL_DEBUG("\nFREE ");

	#ifdef KMALLOC_DEBUG
		char itoa_result[100];
		itoa(ptr, itoa_result, 16);
		serial_print("0x");
		serial_print(itoa_result);
	#endif

	struct mem_block* header = (struct mem_block*)((uint32_t)ptr - sizeof(struct mem_block));

	if((uint32_t)header < alloc_start || (uint32_t)ptr >= alloc_end) {
		SERIAL_DEBUG("INVALID_BOUNDS\n");
		return;
	}

	if(header->magic != KMALLOC_MAGIC) {
		SERIAL_DEBUG("INVALID_MAGIC\n");
		return;
	}

	if(unlikely(!spinlock_get(&kmalloc_lock, 30))) {
		SERIAL_DEBUG("Could not get spinlock\n");
		return NULL;
	}

	// Check previous block
	#if 0
	if((ptr - sizeof(struct mem_block)) > alloc_start) {
		SERIAL_DEBUG("(PBF)");

		uint32_t* prev_size = (struct mem_block*)((uint32_t)header - sizeof(struct mem_block));
		struct mem_block* prev = prev_size - *prev_size;

		if(prev->magic != KMALLOC_MAGIC) {
			SERIAL_DEBUG("kfree() for block preceeded by an invalid block\n");
			return;
		}
	}
	#endif

	// TODO Merge adjacent free blocks
	header->status = false;
	header->type = 0;
	header->pid = 0;

	if(next_free_block_pos > MAX_FREE_BLOCKS) {
		SERIAL_DEBUG("Exceeded MAX_FREE_BLOCKS, losing track of this free block.\n");
		return;
	}

	free_blocks[next_free_block_pos] = header;
	next_free_block_pos++;

	if(next_free_block_pos > last_free_block_pos) {
		last_free_block_pos = next_free_block_pos;
	}

	SERIAL_DEBUG("\n");
	spinlock_release(&kmalloc_lock);
}

uint32_t kmalloc_getMemoryPosition()
{
	return alloc_end;
}

void kmalloc_init()
{
	/* The modules are the last things the bootloader loads after the
	 * kernel. Therefore, we can securely assume that everything after
	 * the last module's end should be free.
	 */
	if(multiboot_info->modsCount > 0) // Do we have at least one module?
		alloc_end = multiboot_info->modsAddr[multiboot_info->modsCount - 1].end;
	else // Guess.
		alloc_end = 15 * 1024 * 1024;

	alloc_start = alloc_end;
	initialized = true;
}


void kmalloc_print_stats() {
	log(LOG_DEBUG, "kmalloc post init stats:\n");
	log(LOG_DEBUG, "=========================\n");
	log(LOG_DEBUG, "Number of blocks: %d\n", num_blocks);
	log(LOG_DEBUG, "Total size of blocks: %d bytes / ~%d MB\n", total_blocks_size, total_blocks_size / (1024*1024));
	log(LOG_DEBUG, "Average allocation size: %d bytes / ~%d KB\n", total_blocks_size / num_blocks, total_blocks_size / num_blocks / 1024);

	uint32_t* last_alloc_size = (uint32_t*)(alloc_end - sizeof(uint32_t));
	struct mem_block* last_header = (struct mem_block*)((uint32_t)last_alloc_size - *last_alloc_size - sizeof(struct mem_block));
	log(LOG_DEBUG, "Last memory allocation size (footer): %d\n", *last_alloc_size);
	log(LOG_DEBUG, "Last memory allocation size: 0x%x\n", last_header->magic);
	log(LOG_DEBUG, "Last memory allocation size (header): %d\n", last_header->size);
}
