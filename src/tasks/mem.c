/* task.c: Task memory allocation & management
 * Copyright © 2011-2019 Lukas Martini
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

#include <tasks/mem.h>
#include <tasks/task.h>
#include <mem/kmalloc.h>
#include <mem/palloc.h>
#include <mem/vmem.h>
#include <errno.h>

void task_add_mem(task_t* task, void* virt_start, void* phys_start,
	uint32_t size, enum task_mem_section section, int flags) {
	bool user = section != TMEM_SECTION_KERNEL;
	bool ro = section == TMEM_SECTION_CODE;
	if(section) {
		vmem_map(task->vmem_ctx, virt_start, phys_start, size, user, ro);
	}

	struct task_mem* alloc = zmalloc(sizeof(struct task_mem));
	alloc->phys_addr = phys_start;
	alloc->virt_addr = virt_start;
	alloc->len = size;
	alloc->section = section;
	alloc->flags = flags;

	alloc->next = task->mem_allocs;
	task->mem_allocs = alloc;
}

/* Called on task page faults. If the fault is in the pages below the current
 * lower end of the stack, expand the stack (up to 512 pages total), and return
 * control to the task. otherwise, return -1 so the fault gets raised.
 */
int task_page_fault_cb(task_t* task, uintptr_t addr) {
	addr = ALIGN_DOWN(addr, PAGE_SIZE);
	uintptr_t stack_lower = TASK_STACK_LOCATION - task->stack_size;
	if(addr >= stack_lower || addr <= TASK_STACK_LOCATION - PAGE_SIZE * 512) {
		return -1;
	}

	int alloc_size = MAX(PAGE_SIZE * 2, stack_lower - addr);
	void* page = zpalloc(alloc_size / PAGE_SIZE);
	task_add_mem(task, (void*)(stack_lower - alloc_size), page, alloc_size,
		TMEM_SECTION_STACK, TASK_MEM_FREE | TASK_MEM_PALLOC | TASK_MEM_FORK);

	task->stack_size += alloc_size;
	return 0;
}

/* Copies data from task memory into kernel memory, or vice versa. Usually, you
 * want to use task_memmap instead, which may invoke this function.
 */
void task_memcpy(task_t* task, void* kaddr, void* addr, size_t ptr_size, bool user_to_kernel) {
	uintptr_t off = 0;
	struct vmem_range* cr;

	while(off < ptr_size) {
		cr = vmem_get_range(task->vmem_ctx, (uintptr_t)addr + off, false);
		uintptr_t paddr = vmem_translate_ptr(cr, (uintptr_t)addr + off, false);
		size_t copy_size = MIN(ptr_size - off, cr->length - (paddr - cr->phys_start));
		if(!copy_size) {
			break;
		}

		if(user_to_kernel) {
			memcpy((void*)paddr, kaddr + off, copy_size);
		} else {
			memcpy(kaddr + off, (void*)paddr, copy_size);
		}

		off += copy_size;
	}
}

/* Convert a userland pointer to something usable in kernel space. If the
 * buffer is contiguous in physical memory, just translate the pointer,
 * otherwise copy using task_memcpy. In this case *copied will be set to true,
 * and you must free the returned buffer after use (also, if you want your
 * changes to be visible in the original userspace buffer, you need to
 * manually copy them back using task_memcpy).
 */
void* task_memmap(task_t* task, void* addr, size_t ptr_size, bool* copied) {
	struct vmem_range* vmem_range = vmem_get_range(task->vmem_ctx, (uintptr_t)addr, false);
	if(!vmem_range) {
		return 0;
	}

	uintptr_t ptr_end = (uintptr_t)addr + ptr_size;
	struct vmem_range* cr = vmem_range;

	for(int i = 1;; i++) {
		uintptr_t virt_end = cr->virt_start + cr->length;
		uintptr_t phys_end = cr->phys_start + cr->length;

		/* We've reached the end of the pointer and the buffer is contiguous in
		 * physical memory, so just pass it directly.
		 */
		if(ptr_end <= virt_end) {
			return (void*)vmem_translate_ptr(vmem_range, (uintptr_t)addr, false);
		}

		// Get next vmem range
		cr = vmem_get_range(task->vmem_ctx, virt_end + PAGE_SIZE * i, false);
		if(!cr) {
			return 0;
		}

		// Check if next range is in adjacent physical pages
		if(cr->phys_start != phys_end) {
			break;
		}
	}

	void* fmb = kmalloc(ptr_size);
	task_memcpy(task, fmb, addr, ptr_size, false);
	*copied = true;
	return fmb;
}

void* task_sbrk(task_t* task, int32_t length) {
	if(length <= 0) {
		return task->sbrk;
	}

	length = ALIGN(length, PAGE_SIZE);

	void* phys_addr = zpalloc(length / PAGE_SIZE);
	if(!phys_addr) {
		sc_errno = ENOMEM;
		return (void*)-1;
	}

	void* virt_addr = task->sbrk;
	task->sbrk += length;

	task_add_mem(task, virt_addr, phys_addr, length, TMEM_SECTION_HEAP,
		TASK_MEM_FORK | TASK_MEM_FREE | TASK_MEM_PALLOC);

	return virt_addr;
}

/* Copy a NULL-terminated array of strings to kernel memory
 * FIXME This will fail if the array or one of its elements crosses a page
 * boundary that is not mapped contiguously in physical/kernel memory.
 */
char** task_copy_strings(task_t* task, char** array, uint32_t* count) {
	int size = 0;

	for(; size < 200; size++) {
		if(!array[size]) {
			break;
		}
	}

	if(size >= 200) {
		return NULL;
	}

	char** new_array = kmalloc(sizeof(char*) * (size + 1));
	int i = 0;
	for(; i < size; i++) {
		new_array[i] = strndup((char*)vmem_translate(task->vmem_ctx, (intptr_t)array[i], false), 200);
	}

	new_array[i] = NULL;

	if(count) {
		*count = size;
	}

	return new_array;
}
