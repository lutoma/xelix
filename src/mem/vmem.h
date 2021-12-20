#pragma once

/* Copyright © 2013-2020 Lukas Martini
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

#include <int/int.h>
#include <mem/paging.h>
#include <stdbool.h>


// Writable
#define VM_RW 1

// Readable by user space
#define VM_USER 2

// pfree() physical memory after context/range is unmapped
#define VM_FREE 4

/* This flag is used internally in task code to indicate the range should be
 * copied in fork() commands
 */
#define VM_TFORK 8

// Allocate on write
#define VM_AOW 16

// Copy on write - implies VM_AOW
#define VM_COW 32

#define VM_NOCOW 64

// Temp hack
#define VM_VALLOC_NO_MAP 128


#define vmem_translate_ptr(range, addr, phys)					\
	(phys ? range->virt_addr : range->phys_addr)				\
	+ (addr - (phys ? range->phys_addr : range->virt_addr))

/* Internal representation of a virtual page allocation. This will get mapped to
 * the hardware form by <arch>-paging.c.
 */
struct vmem_range {
	struct vmem_range* next;
	int flags;
	int cow_flags;
	void* virt_addr;
	void* phys_addr;
	size_t size;

	/* Reference counter for physical allocation in cases where VM_COW and
	 * VM_FREE are set and at least one COW dependent exists.
	 */
	uint16_t* ref_count;
};

struct vmem_context {
	struct vmem_range* ranges;

	// Address of the actual page tables that will be read by the hardware
	struct paging_context* hwdata;
	struct paging_context* hwdata_phys;
};

struct vmem_range* vmem_map(struct vmem_context* ctx, void* virt, void* phys, size_t size, int flags);
#define vmem_map_flat(ctx, start, size, flags) vmem_map(ctx, start, start, size, flags)
struct vmem_range* vmem_get_range(struct vmem_context* ctx, void* addr, bool phys);
void* vmem_get_hwdata(struct vmem_context* ctx);
void vmem_rm_context(struct vmem_context* ctx);
void vmem_init();

static inline void* vmem_translate(struct vmem_context* ctx, void* raddress, bool phys) {
	struct vmem_range* range = vmem_get_range(ctx, raddress, phys);
	if(!range) {
		return 0;
	}
	return vmem_translate_ptr(range, raddress, phys);
}
