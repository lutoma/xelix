/* vm.c: Virtual memory management
 * Copyright © 2011 Fritz Grimpen
 * Copyright © 2013-2018 Lukas Martini
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
#include <memory/kmalloc.h>
#include <print.h>
#include <panic.h>
#include <tasks/scheduler.h>

#define FIND_NODE(node, cond) { \
	while (node && !(cond)) \
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

	/* The task this context is used for. NULL = Kernel context. This is
	 * intentionally not set through an argument to vmem_new, as task_new
	 * requires a memory context, which would create a circular dependency.
	 * Instead, this is manually set by the ELF loader.
	 */
	task_t* task;
};

struct vmem_context *vmem_new()
{
	struct vmem_context *ctx = kmalloc(sizeof(struct vmem_context));

	ctx->first_node = NULL;
	ctx->last_node = NULL;
	ctx->node = NULL;
	ctx->pages = 0;
	ctx->cache = NULL;
	ctx->task = NULL;

	return ctx;
}

/* The second argument to this is actually a task_t*, but we can't make it
 * that trivially as we'd have to include tasks/scheduler.h in the header which
 * creates a circular header dependency.
 *
 * FIXME The task_t struct should be moved to its own header file.
 */
void vmem_set_task(struct vmem_context* ctx, void* task) {
	ctx->task = (task_t*)task;
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
	return retval;
}

void vmem_rm_context(struct vmem_context* ctx) {
	struct vmem_context_node *node = ctx->node;

	/* For some reason, freeing pages after an offset of about ~30000 causes
	 * kmalloc to become corrupted, so use this giant hack to avoid that –
	 * freeing a lot of memory is still better than freeing no memory.
	 * FIXME Should be properly debugged.
	 */
	for (int i = 0; node != NULL; node = node->next, i++)
	{
		if(i >= 30000) {
			break;
		}

		struct vmem_context_node* old_node = (void*)node;
		node = node->next;
		kfree(old_node->page);
		kfree(old_node);
	}

	kfree(ctx->cache);
	kfree(ctx);
}

uint32_t vmem_count_pages(struct vmem_context *ctx)
{
	return ctx->pages;
}

char* vmem_get_name(struct vmem_context* ctx) {
	if(ctx == vmem_kernelContext) {
		return "Kernel context";
	}
	if(ctx->task == NULL) {
		return "Unknown context";
	}
	return ctx->task->name;
}

static cpu_state_t* vmem_handle_fault(cpu_state_t* regs)
{
	uint32_t phys_addr = (uint32_t)regs->eip;
	task_t* running_task = scheduler_get_current();

	char* pgpres = bit_get(regs->errCode, 0) ? " (page present)" : "";
	char* op = bit_get(regs->errCode, 1) ? "write" : "read";
	char* mode = bit_get(regs->errCode, 2) ? "user" : "kernel";
	char* bitoverwrite = bit_get(regs->errCode, 3) ? " (due to reserved bits being overwritten)" : "";
	char* instrfetch = bit_get(regs->errCode, 4) ? " (during instruction fetch)" : "";

	if(vmem_currentContext != vmem_kernelContext && running_task)
	{
		struct vmem_page *pg = vmem_get_page_virt(running_task->memory_context, (void*)GET_PAGE(phys_addr));
		uint32_t virt_addr = (uint32_t)pg->virt_addr + (phys_addr % PAGE_SIZE);

		log(LOG_WARN, "Page fault for %s to 0x%x in process <%s>+%x "
			"at EIP 0x%x (phys 0x%x) of context %s, %s mode%s%s%s. Terminating the task.\n",
			op, regs->cr2, running_task->name, (virt_addr - (uint32_t)running_task->entry),
			virt_addr, phys_addr, vmem_get_name(running_task->memory_context), mode, pgpres,
			instrfetch, bitoverwrite);

		scheduler_terminate_current();
		return regs;
	}

	struct vmem_page *kernel_pg = vmem_get_page_virt(vmem_kernelContext, (void *)GET_PAGE(phys_addr));

	if (kernel_pg->virt_addr == vmem_faultAddress)
	{
		log(LOG_DEBUG, "Received debugging page fault\n");
		return regs;
	}

	panic("Page fault for %s to 0x%x in %s mode%s%s%s\n", op, regs->cr2, mode, pgpres,
			instrfetch, bitoverwrite);
	return regs;
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

/* Initialize kernel context */
void vmem_init()
{
	interrupts_register(14, &vmem_handle_fault);

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
