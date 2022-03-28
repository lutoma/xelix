/* mem.c: Task memory allocation & management
 * Copyright © 2011-2020 Lukas Martini
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
#include <mem/mem.h>
#include <mem/vmem.h>
#include <errno.h>

/* Called on task page faults. If the fault is in the pages below the current
 * lower end of the stack, expand the stack (up to 512 pages total), and return
 * control to the task. otherwise, return -1 so the fault gets raised.
 */
int task_page_fault_cb(task_t* task, void* _addr) {
	uintptr_t addr = (uintptr_t)_addr;
	addr = ALIGN_DOWN(addr, PAGE_SIZE);
	uintptr_t stack_lower = TASK_STACK_LOCATION - task->stack_size;
	if(addr >= stack_lower || addr <= TASK_STACK_LOCATION - PAGE_SIZE * 512) {
		return -1;
	}

	int alloc_size = MAX(PAGE_SIZE * 2, stack_lower - addr);
	void* page = palloc(alloc_size / PAGE_SIZE);
	if(!page) {
		return -1;
	}

	// Only allocate to zero out FIXME deallocate vaddr
	vmem_t vmem;
	if(zvalloc(VA_KERNEL, &vmem, PAGE_SIZE * 2, page, VM_RW) != 0) {
		return -1;
	}

	vmem_map(task->vmem_ctx, (void*)(stack_lower - alloc_size), page,
		alloc_size, VM_USER | VM_RW | VM_FREE | VM_NOCOW | VM_TFORK);

	log(LOG_DEBUG, "task_page_fault_cb adding phys %#x virt %#x\n", page, stack_lower - alloc_size);

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
		cr = vmem_get_range(task->vmem_ctx, addr + off, false);
		void* tpaddr = vmem_translate_ptr(cr, addr + off, false);
		void* paddr = vmem_translate(NULL, tpaddr + off, true);

		size_t copy_size = MIN(ptr_size - off,
			(void*)cr->size - (paddr - (uintptr_t)cr->phys_addr));

		if(!copy_size) {
			break;
		}

		if(user_to_kernel) {
			memcpy(paddr, kaddr + off, copy_size);
		} else {
			memcpy(kaddr + off, paddr, copy_size);
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
	struct vmem_range* vmem_range = vmem_get_range(task->vmem_ctx, addr, false);
	if(!vmem_range) {
		return NULL;
	}

#if 0
	uintptr_t ptr_end = (uintptr_t)addr + ptr_size;
	struct vmem_range* cr = vmem_range;

	for(int i = 1;; i++) {
		void* virt_end = cr->virt_addr + cr->size;
		void* phys_end = cr->phys_addr + cr->size;

		/* We've reached the end of the pointer and the buffer is contiguous in
		 * physical memory, so just pass it directly.
		 */
		if(ptr_end <= (uintptr_t)virt_end) {
			*copied = false;
			return vmem_translate_ptr(vmem_range, addr, false);
		}

		// Get next vmem range
		cr = vmem_get_range(task->vmem_ctx, virt_end + PAGE_SIZE * i, false);
		if(!cr) {
			return NULL;
		}

		// Check if next range is in adjacent physical pages
		if(cr->phys_addr != phys_end) {
			break;
		}
	}
#endif

	void* fmb = kmalloc(ptr_size);
	task_memcpy(task, fmb, addr, ptr_size, false);
	*copied = true;
	return fmb;
}

// Free a task and all associated memory
void task_free(task_t* t) {
	vmem_rm_context(t->vmem_ctx);
	kfree_array(t->environ, t->envc);
	kfree_array(t->argv, t->argc);
	kfree(t);
}

void* task_sbrk(task_t* task, int32_t length) {
	if(length <= 0) {
		return task->sbrk;
	}

	length = ALIGN(length, PAGE_SIZE);

	void* phys_addr = palloc(length / PAGE_SIZE);
	if(!phys_addr) {
		sc_errno = ENOMEM;
		return (void*)-1;
	}

	// Only allocate to zero out FIXME deallocate vaddr
	vmem_t vmem;
	if(zvalloc(VA_KERNEL, &vmem, length / PAGE_SIZE, phys_addr, VM_RW) != 0) {
		sc_errno = ENOMEM;
		return (void*)-1;
	}

	void* virt_addr = task->sbrk;
	task->sbrk += length;

	vmem_map(task->vmem_ctx, virt_addr, phys_addr, length,
		VM_USER | VM_RW | VM_NOCOW | VM_TFORK | VM_FREE);
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
		char* phys = (char*)vmem_translate(task->vmem_ctx, array[i], false);
		new_array[i] = strndup((char*)vmem_translate(NULL, phys, true), 200);
	}

	new_array[i] = NULL;

	if(count) {
		*count = size;
	}

	return new_array;
}
