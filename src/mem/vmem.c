/* vmem.c: Virtual memory management
 * Copyright © 2011 Fritz Grimpen
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

#include "vmem.h"
#include <log.h>
#include <mem/kmalloc.h>
#include <mem/paging.h>
#include <panic.h>
#include <string.h>
#include <tasks/scheduler.h>

int vmem_add_page(struct vmem_context *ctx, struct vmem_page *pg) {
	pg->next = NULL;

	if(!ctx->first_page) {
		ctx->pages = 1;
		ctx->first_page = pg;
		ctx->last_page = pg;
		return 0;
	}

	ctx->last_page->next = pg;
	ctx->last_page = pg;
	++ctx->pages;

	if(ctx->tables) {
		paging_apply(ctx, pg);
	}
	return 0;
}

struct vmem_page* vmem_get_page(struct vmem_context* ctx, void* addr, bool phys) {
	addr = ALIGN_DOWN(addr, PAGE_SIZE);
	struct vmem_page* page = ctx->first_page;

	for(; page; page = page->next) {
		if((phys ? page->phys_addr : page->virt_addr) == addr) {
			return page;
		}
	}
	return NULL;
}

void vmem_rm_context(struct vmem_context* ctx) {
	struct vmem_page* page = ctx->first_page;
	for (uint32_t i = 0; page && i < ctx->pages; i++) {
		struct vmem_page* old_page = page;
		page = page->next;
		kfree(old_page);
	}

	kfree(ctx->tables);
	kfree(ctx);
}

void vmem_map(struct vmem_context* ctx, void* virt_start, void* phys_start, uint32_t size, bool user, bool ro) {
	for(uintptr_t i = 0; i < ALIGN(size, PAGE_SIZE); i += PAGE_SIZE) {
		struct vmem_page *page = zmalloc(sizeof(struct vmem_page));
		page->readonly = ro;
		page->cow = 0;
		page->allocated = 1;
		page->user = user;
		page->virt_addr = virt_start + i;
		page->phys_addr = phys_start + i;
		vmem_add_page(ctx, page);
	}
}

uintptr_t vmem_translate(struct vmem_context* ctx, intptr_t raddress, bool reverse) {
	int diff = raddress % PAGE_SIZE;
	raddress -= diff;

	struct vmem_page* page = vmem_get_page(ctx, (void*)raddress, reverse);

	if(!page) {
		return 0;
	}

	intptr_t v = reverse ? (intptr_t)page->virt_addr : (intptr_t)page->phys_addr;
	return v + diff;
}

void vmem_init() {
	// Initialize kernel context
	struct vmem_context *ctx = zmalloc(sizeof(struct vmem_context));
	vmem_map_flat(ctx, 0, 0xffffe000U, 0, 0);
	vmem_kernelContext = ctx;
}
