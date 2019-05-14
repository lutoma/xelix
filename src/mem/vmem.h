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

#define PAGE_SIZE 4096

/* Internal representation of a node. This will get mapped to the hardware form
 * by <arch>-paging.c.
 */
struct vmem_page {
	struct vmem_page* next;
	bool readonly:1;
	bool cow:1; /* Copy-on-Write mechanism */
	bool allocated:1;
	bool user:1;

	void* cow_src_addr;
	void* virt_addr;
	void* phys_addr;
};

struct vmem_context {
	struct vmem_page* first_page;
	struct vmem_page* last_page;
	uint32_t pages;

	// Address of the actual page tables that will be read by the hardware
	void* tables;
};

struct vmem_context* vmem_kernelContext;

struct vmem_page *vmem_new_page();
int vmem_add_page(struct vmem_context *ctx, struct vmem_page *pg);
struct vmem_page* vmem_get_page(struct vmem_context* ctx, void* addr, bool phys);
void vmem_rm_context(struct vmem_context* ctx);
void vmem_map(struct vmem_context* ctx, void* virt_start, void* phys_start, uint32_t size, bool user, bool ro);
#define vmem_map_flat(ctx, start, size, user, ro) vmem_map(ctx, start, start, size, user, ro)
uintptr_t vmem_translate(struct vmem_context* ctx, intptr_t raddress, bool reverse);
void vmem_init();
