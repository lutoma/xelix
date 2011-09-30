#pragma once

/* Copyright © 2011 Fritz Grimpen
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

#include <lib/generic.h>

struct vm_context;

struct vm_page
{
	enum
	{
		VM_SECTION_STACK, /* Initial stack */
		VM_SECTION_CODE,  /* Contains program code and is read-only */
		VM_SECTION_DATA,  /* Contains static data */
		VM_SECTION_HEAP,  /* Allocated by brk(2) at runtime */
		VM_SECTION_MMAP,  /* Allocated by mmap(2) at runtime */
		VM_SECTION_KERNEL /* Contains kernel-internal data */
	} section;

	bool readonly:1;
	bool cow:1; /* Copy-on-Write mechanism */

	void *cow_src_addr;
	void *virt_addr;
	void *phys_addr;
};

/* Generate new page context */
struct vm_context *vm_new();

int vm_add_page(struct vm_context *ctx, struct vm_page *pg);

struct vm_page *vm_get_page_phys(struct vm_context *ctx, void *phys_addr);
struct vm_page *vm_get_page_virt(struct vm_context *ctx, void *virt_addr);

int vm_get_pages(struct vm_context *ctx, struct vm_page **dst, int size);

/* Remove pages in a specific context by physical or virtual address */
struct vm_page *vm_rm_page_phys(struct vm_context *ctx, void *phys_addr);
struct vm_page *vm_rm_page_virt(struct vm_context *ctx, void *virt_addr);
