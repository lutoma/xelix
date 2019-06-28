#pragma once

/* Copyright © 2011 Fritz Grimpen
 * Copyright © 2013-2019 Lukas Martini
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
#include <stdbool.h>

#define PAGE_SIZE 0x1000

#define vmem_translate_ptr(range, addr, phys)					\
	(phys ? range->virt_start : range->phys_start)				\
	+ (addr - (phys ? range->phys_start : range->virt_start))

/* Internal representation of a page allocation. This will get mapped to the
 * hardware form by <arch>-paging.c.
 */
struct vmem_range {
	struct vmem_range* next;
	bool readonly:1;
	bool user:1;

	uintptr_t virt_start;
	uintptr_t phys_start;
	uintptr_t length;
};

struct vmem_context {
	struct vmem_range* ranges;

	// Address of the actual page tables that will be read by the hardware
	struct paging_context* hwdata;
};

/* Used in interrupt handlers to return to kernel paging context */
void* vmem_kernel_hwdata;

void vmem_map(struct vmem_context* ctx, void* virt_start, void* phys_start, uintptr_t size, bool user, bool ro);
#define vmem_map_flat(ctx, start, size, user, ro) vmem_map(ctx, start, start, size, user, ro)
struct vmem_range* vmem_get_range(struct vmem_context* ctx, uintptr_t addr, bool phys);
void* vmem_get_hwdata(struct vmem_context* ctx);
void vmem_rm_context(struct vmem_context* ctx);
void vmem_init();

static inline uintptr_t vmem_translate(struct vmem_context* ctx, uintptr_t raddress, bool phys) {
	struct vmem_range* range = vmem_get_range(ctx, raddress, phys);
	if(!range) {
		return 0;
	}
	return vmem_translate_ptr(range, raddress, phys);
}
