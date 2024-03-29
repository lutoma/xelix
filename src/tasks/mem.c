/* mem.c: Task memory allocation & management
 * Copyright © 2011-2023 Lukas Martini
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
#include <errno.h>

#define PROT_NONE 1
#define PROT_READ 2
#define PROT_WRITE 4
#define PROT_EXEC 8

#define MAP_PRIVATE 1
#define MAP_SHARED 2
#define MAP_ANONYMOUS 4
#define MAP_FIXED 8

static int task_stack_grow(task_t* task, size_t alloc_size) {
	if(task->stack_size + alloc_size > PAGE_SIZE * 512) {
		sc_errno = ENOMEM;
		return -1;
	}

	uintptr_t stack_lower = TASK_STACK_LOCATION - task->stack_size;
	if(!vm_alloc_at(&task->vmem, NULL, RDIV(alloc_size, PAGE_SIZE), (void*)(stack_lower - alloc_size), NULL,
		VM_USER | VM_RW | VM_FREE | VM_NOCOW | VM_TFORK | VM_ZERO | VM_FIXED)) {
		return -1;
	}

	task->stack_size += alloc_size;
	return 0;
}

/* Called on task page faults. If the fault is in the pages below the current
 * lower end of the stack, expand the stack (up to 512 pages total), and return
 * control to the task. otherwise, return -1 so the fault gets raised.
 */
int task_page_fault_cb(task_t* task, void* _addr) {
	uintptr_t addr = (uintptr_t)_addr;
	addr = ALIGN_DOWN(addr, PAGE_SIZE);

	uintptr_t stack_lower = TASK_STACK_LOCATION - task->stack_size;
	if(addr >= stack_lower) {
		return -1;
	}

	int alloc_size = stack_lower - addr + PAGE_SIZE;
	return task_stack_grow(task, alloc_size);
}

// Free a task and all associated memory
void task_free(task_t* t) {
	vm_cleanup(&t->vmem);
	kfree_array(t->environ, t->envc);
	kfree_array(t->argv, t->argc);
	kfree(t);
}

void* task_sbrk(task_t* task, int32_t length) {
	if(length <= 0) {
		return task->sbrk;
	}

	length = ALIGN(length, PAGE_SIZE);

	void* virt_addr = task->sbrk;
	task->sbrk += length;

	if(!vm_alloc_at(&task->vmem, NULL, RDIV(length, PAGE_SIZE), virt_addr, NULL,
		VM_USER | VM_RW | VM_NOCOW | VM_TFORK | VM_FREE | VM_FIXED)) {
		return (void*)-1;
	}

	return virt_addr;
}

void* task_mmap(task_t* task, struct task_mmap_ctx* ctx) {
	if(ctx->len == 0) {
		sc_errno = EINVAL;
		return NULL;
	}

	if(!(ctx->flags & MAP_ANONYMOUS)) {
		sc_errno = ENOSYS;
		return NULL;
	}

	if(ctx->flags & MAP_SHARED ) {
		sc_errno = ENOTSUP;
		return NULL;
	}

	if(ctx->prot & PROT_NONE || !(ctx->prot & PROT_READ)) {
		sc_errno = ENOTSUP;
		return NULL;
	}

	int vaflags = VM_USER | VM_NOCOW | VM_TFORK | VM_FREE;
	if(ctx->prot & PROT_WRITE) {
		vaflags |= VM_RW;
	}

	void* req = ctx->addr;
	if(ctx->flags & MAP_FIXED) {
		if(!req) {
			sc_errno = EINVAL;
			return NULL;
		}

		vaflags |= VM_FIXED;
	}

	/* When request is unset, start allocating at an arbitrary high address
	 * to hopefully avoid conflicts with future sbrk allocations.
	 */
	if(!req) {
		req = (void*)CONFIG_MMAP_BASE;
	}

	void* addr = vm_alloc_at(&task->vmem, NULL, RDIV(ctx->len, PAGE_SIZE), req, NULL, vaflags);
	if(!addr) {
		return (void*)-1;
	}

	return addr;
}

/* Copy a NULL-terminated array of strings to kernel memory.
 * Max string length: VFS_PATH_MAX. Used for execve args.
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

	size_t max_length = VFS_PATH_MAX;
	char** new_array = kmalloc(sizeof(char*) * (size + 1));
	int i = 0;
	for(; i < size; i++) {
		vm_alloc_t vmem;
		char* old_string = vm_map(VM_KERNEL, &vmem, &task->vmem, array[i], VFS_PATH_MAX, 0);

		if(!old_string) {
			// Retry with shorter length to stay within the page
			max_length = ALIGN(array[i], PAGE_SIZE) - array[i] - 1;
			old_string = vm_map(VM_KERNEL, &vmem, &task->vmem, array[i], max_length, 0);
			if(!old_string) {
				return NULL;
			}
		}

		new_array[i] = strndup(old_string, max_length);
		vm_free(&vmem);

		// Make sure string is NULL-terminated
		size_t slen = strnlen(new_array[i], max_length);
		if(slen == max_length) {
			log(LOG_WARN, "task_copy_strings: %d %s: Unterminated string in array\n",
				task->pid, task->name);
			task_signal(task, NULL, SIGSEGV);
			return NULL;
		}
	}

	new_array[i] = NULL;

	if(count) {
		*count = size;
	}

	return new_array;
}
