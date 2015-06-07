/* vm.c: Virtual memory management
 * Copyright © 2011 Fritz Grimpen
 * Copyright © 2013 Lukas Martini
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
#include <lib/log.h>
#include <memory/kmalloc.h>
#include <lib/print.h>
#include <lib/panic.h>

#define FIND_NODE(node, cond) { \
	while (!(cond) && node != NULL) \
		node = node->next; \
}

#define GET_PAGE(a) (a - (a % PAGE_SIZE))

struct vmem_context_node
{
	struct vmem_page *page;
	struct vmem_context_node *next;
};

struct vmem_context
{
	union {
		struct vmem_context_node *first_node;
		struct vmem_context_node *node;
	};
	struct vmem_context_node *last_node;
	union {
		uint32_t pages;
		uint32_t nodes;
	};

	void *cache;
};

/* Initialize kernel context */
void vmem_init()
{
	struct vmem_context *ctx = vmem_new();
	
	vmem_faultAddress = kmalloc_a(PAGE_SIZE);

	struct vmem_page *debugPage = vmem_new_page();
	debugPage->section = VMEM_SECTION_UNMAPPED;
	debugPage->virt_addr = vmem_faultAddress;

	vmem_add_page(ctx, debugPage);
	
	for (char *i = (char*)0; i <= (char*)0xffffe000U; i += 4096)
	{
		if (i == vmem_faultAddress)
			continue;
		struct vmem_page *page = vmem_new_page();
		page->section = VMEM_SECTION_KERNEL;
		page->cow = 0;
		page->allocated = 1;
		page->virt_addr = (void *)i;
		page->phys_addr = (void *)i;

		vmem_add_page(ctx, page);
	}

	vmem_kernelContext = ctx;
}

struct vmem_context *vmem_new()
{
	struct vmem_context *ctx = kmalloc(sizeof(struct vmem_context));

	ctx->first_node = NULL;
	ctx->last_node = NULL;
	ctx->node = NULL;
	ctx->pages = 0;
	ctx->cache = NULL;

	return ctx;
}

struct vmem_page *vmem_new_page()
{
	struct vmem_page *page = kmalloc(sizeof(struct vmem_page));
	memset(page, 0, sizeof(struct vmem_page));
	return page;
}

int vmem_iterate(struct vmem_context *ctx, vmem_iterator_t iterator)
{
	struct vmem_context_node *node = ctx->first_node;
	int i = 0;
	while (node != NULL && i < ctx->nodes)
	{
		iterator(ctx, node->page, i++);
		node = node->next;
	}

	return i;
}

int vmem_add_page(struct vmem_context *ctx, struct vmem_page *pg)
{
	struct vmem_context_node *node = kmalloc(sizeof(struct vmem_context_node));
	node->page = pg;
	node->next = NULL;
	if (ctx->node == NULL)
	{
		ctx->pages = 1;
		ctx->first_node = node;
		ctx->last_node = node;
		ctx->node = node;
		return 0;
	}

	ctx->last_node->next = node;
	ctx->last_node = node;
	++ctx->pages;

	if (ctx->cache != NULL)
		vmem_applyPage(ctx, pg);

	return 0;
}

struct vmem_page *vmem_get_page_phys(struct vmem_context *ctx, void *phys_addr)
{
	phys_addr = VMEM_ALIGN_DOWN(phys_addr);
	struct vmem_context_node *node = ctx->node;

	FIND_NODE(node, node->page->phys_addr == phys_addr);

	if (node == NULL)
		return NULL;
	return node->page;
}

struct vmem_page *vmem_get_page_virt(struct vmem_context *ctx, void *virt_addr)
{
	virt_addr = VMEM_ALIGN_DOWN(virt_addr);
	struct vmem_context_node *node = ctx->node;

	FIND_NODE(node, node->page->virt_addr == virt_addr);

	if (node == NULL)
		return NULL;
	return node->page;
}

struct vmem_page *vmem_get_page(struct vmem_context *ctx, uint32_t offset)
{
	struct vmem_context_node *node = ctx->node;

	while (offset > 0 && node != NULL)
	{
		node = node->next;
		--offset;
	}

	if (node == NULL)
		return NULL;
	return node->page;
}

struct vmem_page *vmem_rm_page_phys(struct vmem_context *ctx, void *phys_addr)
{
	phys_addr = VMEM_ALIGN_DOWN(phys_addr);
	struct vmem_context_node *node = ctx->node;
	struct vmem_context_node *prev_node = NULL;

	while (node != NULL && node->page->phys_addr != phys_addr)
	{
		prev_node = node;
		node = node->next;
	}

	if (node == NULL)
		return NULL;

	if (node->next == NULL)
		ctx->last_node = (prev_node != NULL) ? prev_node : NULL;
	if (prev_node == NULL)
		ctx->first_node = NULL;
	else
		prev_node->next = node->next;

	--ctx->pages;
	struct vmem_page *retval = node->page;
	kfree(node);
	
	return retval;
}

struct vmem_page *vmem_rm_page_virt(struct vmem_context *ctx, void *virt_addr)
{
	virt_addr = VMEM_ALIGN_DOWN(virt_addr);
	struct vmem_context_node *node = ctx->node;
	struct vmem_context_node *prev_node = NULL;

	while (node != NULL && node->page->virt_addr != virt_addr)
	{
		prev_node = node;
		node = node->next;
	}

	if (node == NULL)
		return NULL;

	if (node->next == NULL)
		ctx->last_node = (prev_node != NULL) ? prev_node : NULL;
	if (prev_node == NULL)
		ctx->first_node = NULL;
	else
		prev_node->next = node->next;

	--ctx->pages;
	
	struct vmem_page *retval = node->page;
	kfree(node);
	
	return retval;
}

uint32_t vmem_count_pages(struct vmem_context *ctx)
{
	return ctx->pages;
}

void vmem_handle_fault(uint32_t code, void *addr)
{
	uint32_t addrInt = (uint32_t)addr;
	struct vmem_page *pg = vmem_get_page_virt(vmem_currentContext, (void *)GET_PAGE(addrInt));

	task_t* running_task = scheduler_get_current();
	if(running_task)
	{
		log(LOG_WARN, "Segmentation fault in task %s "
			"at address 0x%x. Terminating it.\n",
			running_task->name, addrInt);

		scheduler_terminate_current();
		return;
	}

	if (pg->virt_addr == vmem_faultAddress)
	{
		log(LOG_DEBUG, "Received debugging page fault\n");
		return;
	}

	if (pg == NULL || pg->section == VMEM_SECTION_UNMAPPED)
		panic("Unexpected page fault\n");
}

void vmem_set_cache(struct vmem_context *ctx, void *cache)
{
	ctx->cache = cache;
}

void *vmem_get_cache(struct vmem_context *ctx)
{
	return ctx->cache;
}

static void vmem_dump_page_internal(struct vmem_context *ctx, struct vmem_page *pg, uint32_t i)
{
	char *typeString = "UNKNOWN";
	switch (pg->section)
	{
		case VMEM_SECTION_STACK:
			typeString = "STACK";
			break;
		case VMEM_SECTION_CODE:
			typeString = "CODE";
			break;
		case VMEM_SECTION_DATA:
			typeString = "DATA";
			break;
		case VMEM_SECTION_HEAP:
			typeString = "HEAP";
			break;
		case VMEM_SECTION_MMAP:
			typeString = "MMAP";
			break;
		case VMEM_SECTION_KERNEL:
			typeString = "KERNEL";
			break;
		case VMEM_SECTION_UNMAPPED:
			typeString = "UNMAPPED";
			break;
	}

	printf("[%s] Virt 0x%x Phys 0x%x Cow 0x%x ro:%d alloc:%d cow:%d\n",
			typeString,
			pg->virt_addr,
			pg->phys_addr,
			pg->cow_src_addr,
			pg->readonly,
			pg->allocated,
			pg->cow);
}

void vmem_dump(struct vmem_context *ctx)
{
	vmem_iterate(ctx, vmem_dump_page_internal);
}

void vmem_dump_page(struct vmem_page *pg)
{
	vmem_dump_page_internal(NULL, pg, 0);
}
