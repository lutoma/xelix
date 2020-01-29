/* kmalloc.c: Kernel memory allocator
 * Copyright © 2016-2020 Lukas Martini
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
#include "palloc.h"
#include <log.h>
#include <string.h>
#include <panic.h>
#include <spinlock.h>

#define GET_FOOTER(x) ((struct footer*)((uintptr_t)x + x->size + sizeof(struct mem_block)))
#define GET_CONTENT(x) ((void*)((uintptr_t)x + sizeof(struct mem_block)))
#define GET_FB(x) ((struct free_block*)GET_CONTENT(x))
#define GET_HEADER_FROM_FB(x) ((struct mem_block*)((uintptr_t)(x) - sizeof(struct mem_block)))
#define PREV_FOOTER(x) ((uint32_t*)((uintptr_t)x - sizeof(struct footer)))
#define PREV_BLOCK(x) ((struct mem_block*)((uintptr_t)PREV_FOOTER(x) \
	- (*PREV_FOOTER(x)) - sizeof(struct mem_block)))
#define NEXT_BLOCK(x) ((struct mem_block*)((uintptr_t)GET_FOOTER(x) + sizeof(struct footer)))
#define FULL_SIZE(x) (x->size + sizeof(struct footer) + sizeof(struct mem_block))

/* Enable debugging. This will send out cryptic debug codes to the serial line
 * during kmalloc()/free()'s. Also makes everything horribly slow. */
#ifdef KMALLOC_DEBUG
	#define debug(args...) log(LOG_DEBUG, args)
	#define DEBUGREGS , char* _debug_file, uint32_t _debug_line, const char* _debug_func
#else
	#define debug(args...)
	#define DEBUGREGS
#endif

/* Simple bounds/buffer overflow checking by inserting canaries at the
 * beginning and end of all internal structures (which are placed before and
 * after the actual allocations and tend to get overwritten on buffer overflow).
 * Wastes memory and makes things slow. Only use during development.
 */
#ifdef KMALLOC_CHECK
	#define KMALLOC_CANARY 0xCAFE
	#define _SET_CANARIES(x, y) (x)->canary1 = y; (x)->canary2 = y
	#define SET_CANARIES(x) _SET_CANARIES(x, KMALLOC_CANARY)
	#define CLEAR_CANARIES(x) _SET_CANARIES(x, 0)
	#define CANARY(x) uint16_t canary ## x
#else
	#define SET_CANARIES(x)
	#define CLEAR_CANARIES(x)
	#define CANARY(x)
#endif

/* This is the block header struct. It is always located directly before the
 * start of the allocated area. Following the allocated area, there is a single
 * uint32_t containing the length of the block. As a result, any block header
 * except the first should always have the length of the previous block
 * directly before it. This makes it possible to use these blocks as a doubly
 * linked list.
 */
struct mem_block {
	CANARY(1);
	uint32_t size;

	enum {
		TYPE_USED,
		TYPE_FREE
	} type;
	CANARY(2);
} __aligned(8);

/* For free blocks, this struct gets stored inside the allocated area. As a
 * side effect of this, the minimum size for allocations is the size of this
 * struct.
 */
struct free_block {
	CANARY(1);
	struct free_block* prev;
	struct free_block* next;
	CANARY(2);
} __aligned(8);

struct footer {
	uint32_t size;
} __aligned(8);


#ifdef KMALLOC_CHECK
	static void check_header(struct mem_block* header, bool recurse);
#else
	#define check_header(...)
#endif

bool kmalloc_ready = false;
static spinlock_t kmalloc_lock;
static struct free_block* last_free = (struct free_block*)NULL;
static uintptr_t alloc_start;
static uintptr_t alloc_end;
static uintptr_t alloc_max;

static inline void unlink_free_block(struct free_block* fb) {
	if(fb->next) {
		fb->next->prev = fb->prev;
	}

	if(fb->prev) {
		fb->prev->next = fb->next;
	}

	if(fb == last_free) {
		last_free = fb->prev;
	}
}

static inline struct mem_block* set_block(size_t sz, struct mem_block* header) {
	header->size = sz;
	SET_CANARIES(header);

	struct footer* footer = GET_FOOTER(header);
	footer->size = header->size;
	return header;
}

static struct mem_block* free_block(struct mem_block* header, bool check_next) {
	struct mem_block* prev = PREV_BLOCK(header);
	struct free_block* fb = GET_FB(header);

	/* If previous block is free, just increase the size of that block to also
	 * cover this area. Otherwise write free block metadata and add block.
	 */
	if((uintptr_t)header > alloc_start && prev->type == TYPE_FREE) {
		CLEAR_CANARIES(header);
		header = set_block(prev->size + FULL_SIZE(header), prev);
	} else {
		header->type = TYPE_FREE;

		fb->prev = last_free;
		fb->next = (struct free_block*)NULL;
		SET_CANARIES(fb);

		if(last_free) {
			last_free->next = fb;
		}

		last_free = fb;
	}

	// If next block is free, increase block size and unlink the next fb.
	struct mem_block* next = NEXT_BLOCK(header);
	if(check_next && alloc_end > (uintptr_t)next && next->type == TYPE_FREE) {
		set_block(header->size + FULL_SIZE(next), header);
		unlink_free_block(GET_FB(next));
		CLEAR_CANARIES(next);
	}

	return header;
}

static inline struct mem_block* split_block(struct mem_block* header, size_t sz) {
	// Make sure the block is big enough to get split first
	if(header->size < sz + sizeof(struct mem_block)
		+ sizeof(struct footer) + sizeof(struct free_block)) {

		return NULL;
	}

	size_t orig_size = header->size;
	set_block(sz, header);
	size_t new_size = orig_size - sz - sizeof(struct mem_block) - sizeof(struct footer);
	return set_block(new_size, NEXT_BLOCK(header));
}

static size_t get_alignment_offset(void* address) {
	size_t offset = 0;
	uintptr_t content_addr = (uintptr_t)GET_CONTENT(address);

	// Check if page is not already page aligned by circumstance
	if(content_addr & (PAGE_SIZE - 1)) {
		offset = ALIGN(content_addr, PAGE_SIZE) - content_addr;

		/* We need at least x bytes to store the headers and footers of our
		 * block and of the new block we'll create in the offset
		 * FIXME Calc proper value for minimum size
	 	 */
		if(offset < 0x100) {
			offset += PAGE_SIZE;
		}
	}

	return offset;
}

static inline struct mem_block* get_free_block(size_t sz, bool align) {
	debug("FFB ");

	for(struct free_block* fb = last_free; fb; fb = fb->prev) {
		struct mem_block* fblock = GET_HEADER_FROM_FB(fb);
		check_header(fblock, true);

		if(unlikely(fblock->type != TYPE_FREE)) {
			log(LOG_ERR, "kmalloc: Non-free block in free blocks linked list?\n");
			continue;
		}

		size_t sz_needed = sz;
		uint32_t alignment_offset = 0;

		/* For aligned blocks, special care needs to be taken as usually, the
		 * free block will have to be split up to an offset block and the
		 * actual allocation. This changes our space requirements – We now need
		 * a block with a content size big enough for the full size of the
		 * offset header (variable depending on address, but needs to be at
		 * least block header + footer size + minimum block size).
		 *
		 * Regardless of alignment, if our required size is smaller than the
		 * free block, we will split the free block into our allocation and a
		 * remainder. We also need to ensure the remainder is not smaller than
		 * the minimum block size.
		 */
		if(align) {
			alignment_offset = get_alignment_offset(fblock);
			sz_needed += alignment_offset + sizeof(struct mem_block) + sizeof(struct footer);
		}

		if(fblock->size >= sz_needed) {
			debug("HIT 0x%x size 0x%x ", fblock, fblock->size);
			unlink_free_block(fb);

			// Carve a chunk of the required size out of the block
			struct mem_block* new = split_block(fblock, sz + alignment_offset);

			if(new) {
				// Already set this to prevent free_block from merging
				fblock->type = TYPE_USED;
				free_block(new, true);
			}

			return fblock;
		}
	}

	return NULL;
}

void* __attribute__((alloc_size(1))) _kmalloc(size_t sz, bool align, bool zero DEBUGREGS) {
	if(unlikely(!kmalloc_ready)) {
		panic("Attempt to kmalloc before allocator is kmalloc_ready.\n");
	}

	debug("kmalloc: %s:%d %s %#x ", _debug_file, _debug_line, _debug_func, sz);

	// Ensure size is byte-aligned and no smaller than minimum
	size_t sz_needed = ALIGN(sz, 8);
	sz_needed = MAX(sz_needed, sizeof(struct free_block));

	if(unlikely(!spinlock_get(&kmalloc_lock, 30))) {
		debug("Could not get spinlock\n");
		return NULL;
	}

	struct mem_block* header = get_free_block(sz_needed, align);
	size_t alignment_offset = 0;

	if(align) {
		alignment_offset = get_alignment_offset(header ? header : (struct mem_block*)alloc_end);
	}

	if(!header) {
		debug("NEW alloc_end=%#x ", alloc_end);

		if(alloc_end + sz_needed >= alloc_max) {
			panic("kmalloc: Out of memory");
		}

		if(align && alignment_offset) {
			sz_needed += get_alignment_offset((struct mem_block*)alloc_end);
		}

		header = set_block(sz_needed, (struct mem_block*)alloc_end);
		alloc_end = (uint32_t)GET_FOOTER(header) + sizeof(struct footer);
	}

	if(align && alignment_offset) {
		debug("ALIGN off 0x%x ", alignment_offset);

		struct mem_block* new = split_block(header, alignment_offset
			- sizeof(struct mem_block) - sizeof(struct footer));

		new->type = TYPE_USED;
		free_block(header, true);
		header = new;
	}

	header->type = TYPE_USED;
	spinlock_release(&kmalloc_lock);

	if(zero) {
		bzero((void*)GET_CONTENT(header), sz);
	}

	check_header(header, true);
	debug("RESULT 0x%x\n", (uintptr_t)GET_CONTENT(header));
	return (void*)GET_CONTENT(header);
}

void _kfree(void *ptr DEBUGREGS) {
	if(!ptr) {
		return;
	}

	struct mem_block* header = (struct mem_block*)((uintptr_t)ptr
		- sizeof(struct mem_block));

	debug("kfree: %s:%d %s 0x%x size 0x%x\n", _debug_file, _debug_line,
		_debug_func, ptr, header->size);
	if(unlikely((uintptr_t)header < alloc_start ||
		(uintptr_t)ptr >= alloc_end || header->type == TYPE_FREE)) {

		log(LOG_ERR, "kmalloc: Attempt to free invalid block\n");
		return;
	}

	check_header(header, true);
	if(unlikely(!spinlock_get(&kmalloc_lock, 30))) {
		debug("Could not get spinlock\n");
		return;
	}

	free_block(header, true);
	spinlock_release(&kmalloc_lock);
}

void kmalloc_init() {
	alloc_start = (uintptr_t)palloc(0x6400);
	alloc_end = alloc_start;
	alloc_max = (uintptr_t)alloc_start + (0x6400 * PAGE_SIZE);
	kmalloc_ready = true;
	log(LOG_DEBUG, "kmalloc: Allocating from %#x - %#x\n", alloc_start, alloc_max);
}

void kmalloc_get_stats(uint32_t* total, uint32_t* used) {
	*total = alloc_max - alloc_start;
	*used = alloc_end - alloc_start;
	for(struct free_block* fb = last_free; fb; fb = fb->prev) {
		struct mem_block* fblock = GET_HEADER_FROM_FB(fb);
		*used -= fblock->size;
	}
}

#ifdef KMALLOC_CHECK
#define check_err(fmt, args...) \
	panic("kmalloc: Metadata corruption at 0x%x: " fmt "\n", header, ##args);

#define check_canaries(x)										\
	if(unlikely((x)->canary1 != KMALLOC_CANARY)) {				\
		check_err("Invalid " #x " start canary (%#x != %#x)",	\
			(x)->canary1, KMALLOC_CANARY);						\
	}															\
																\
	if(unlikely((x)->canary2 != KMALLOC_CANARY)) {				\
		check_err("Invalid " #x " end canary (%#x != %#x)",		\
			(x)->canary1, KMALLOC_CANARY);						\
	}

static void check_header(struct mem_block* header, bool recurse) {
	if(unlikely(!header || (uintptr_t)header < alloc_start || (uintptr_t)header > alloc_end)) {
		check_err("Allocation out of bounds");
	}

	check_canaries(header);
	if(header->type == TYPE_FREE) {
		struct free_block* free_block = GET_FB(header);
		check_canaries(free_block);
	}

	int min_sz = sizeof(struct free_block);
	if(unlikely(header->size < min_sz)) {
		check_err("Block is smaller than minimum size (%d < %d)",
			header->size, min_sz);
	}

	struct footer* footer = GET_FOOTER(header);
	if(unlikely(header->size != footer->size)) {
		check_err("Header size doesn't match footer size (%d != %d)",
			header->size, footer->size);
	}

	if(likely((uintptr_t)header != alloc_start) && recurse) {
		check_header(PREV_BLOCK(header), false);
	}

	if(likely(alloc_end > (uintptr_t)header + FULL_SIZE(header)) && recurse) {
		check_header(NEXT_BLOCK(header), false);
	}
}
#endif

#ifdef KMALLOC_DEBUG
void kmalloc_stats() {
	struct mem_block* header = (struct mem_block*)alloc_start;
	log(LOG_DEBUG, "\nkmalloc_stats():\n");
	for(; (uintptr_t)header < alloc_end; header = NEXT_BLOCK(header)) {
		check_header(header, false);

		log(LOG_DEBUG, "0x%x\tsize 0x%x\tres 0x%x\t", header, header->size,
				(uintptr_t)header + sizeof(struct mem_block));
		log(LOG_DEBUG, "fsz 0x%x\tend 0x%x\t ", FULL_SIZE(header),
			(uintptr_t)header + FULL_SIZE(header));

		if(header->type == TYPE_FREE) {
			struct free_block* fb = GET_FB(header);
			log(LOG_DEBUG, "free\tprev free: 0x%x next: 0x%x", fb->prev, fb->next);
		} else {
			log(LOG_DEBUG, "used");
		}

		log(LOG_DEBUG, "\n");
	}

	log(LOG_DEBUG, "\nalloc end:\t0x%x\n", alloc_end);
	log(LOG_DEBUG, "last free:\t0x%x\n\n", last_free);
}
#endif
