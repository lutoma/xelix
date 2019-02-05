/* task.c: Userland tasks
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

#include <tasks/task.h>
#include <tasks/execdata.h>
#include <memory/vmem.h>
#include <memory/kmalloc.h>
#include <memory/paging.h>
#include <memory/gdt.h>
#include <hw/interrupts.h>
#include <fs/sysfs.h>
#include <string.h>
#include <errno.h>

#define STACKSIZE PAGE_SIZE

uint32_t highest_pid = 0;
extern void* __kernel_start;
extern void* __kernel_end;

static size_t sfs_read(void* dest, size_t size, size_t offset, void* meta);

void task_add_mem(task_t* task, void* virt_start, void* phys_start,
	uint32_t size, enum vmem_section section, int flags) {
	if(section && section != VMEM_SECTION_UNMAPPED) {
		vmem_map(task->memory_context, virt_start, phys_start, size, section);
	}

	struct task_mem* alloc = zmalloc(sizeof(struct task_mem));
	alloc->phys_addr = phys_start;
	alloc->virt_addr = virt_start;
	alloc->len = size;
	alloc->section = section;
	alloc->flags = flags;

	alloc->next = task->memory_allocations;
	task->memory_allocations = alloc;
}

static task_t* alloc_task() {
	task_t* task = (task_t*)zmalloc(sizeof(task_t));
	task->pid = ++highest_pid;
	task->task_state = TASK_STATE_RUNNING;
	task->interrupt_yield = false;

	// State gets mapped into user memory, so needs to be one full page.
	task->state = zmalloc_a(PAGE_SIZE);
	task->stack = zmalloc_a(STACKSIZE);
	task->kernel_stack = zmalloc_a(STACKSIZE);
	task->memory_context = vmem_new();
	vmem_set_task(task->memory_context, task);
	return task;
}

/* Sets up a new task, including the necessary paging context, stacks,
 * interrupt stack frame etc. The binary still has to be mapped into the paging
 * context separately (usually in the ELF loader).
 */
task_t* task_new(void* entry, task_t* parent, char name[TASK_MAXNAME],
	char** environ, uint32_t envc, char** argv, uint32_t argc) {

	task_t* task = alloc_task();
	strcpy(task->name, name);
	task->entry = entry;
	task->parent = parent;
	task->environ = environ;
	task->envc = envc;
	task->argv = argv;
	task->argc = argc;

	if(parent)
		memcpy(task->cwd, parent->cwd, TASK_PATH_MAX);
	else
		strcpy(task->cwd, "/");

	/* 1:1 map required memory regions:
	 *  - Task stack
	 *  - Kernel task stack (for TSS).
	 *  - Task state struct (used in the interrupt return).
	 *  - Kernel memory
	 *
	 * Todo: Investigate if we can put all the kernel code that runs before
	 * the paging context switch in a separate ELF section and maybe only map
	 * that.
	 */
	task_add_mem_flat(task, task->stack, STACKSIZE, VMEM_SECTION_DATA, TASK_MEM_FREE);
	task_add_mem_flat(task, task->kernel_stack, STACKSIZE, VMEM_SECTION_KERNEL, TASK_MEM_FREE);
	task_add_mem_flat(task, task->state, PAGE_SIZE, VMEM_SECTION_DATA, TASK_MEM_FREE);

	void* kernel_start = VMEM_ALIGN_DOWN(&__kernel_start);
	uint32_t kernel_size = (uint32_t)&__kernel_end - (uint32_t)kernel_start;
	task_add_mem_flat(task, kernel_start, kernel_size, VMEM_SECTION_KERNEL, 0);

	//*(task->state->ebp + 1) = (intptr_t)scheduler_terminate_current;
	//*(task->state->ebp + 2) = (intptr_t)NULL; // base pointer

	task->state->ds = GDT_SEG_DATA_PL3;
	task->state->cr3 = (uint32_t)paging_get_context(task->memory_context);
	task->state->ebp = task->stack + STACKSIZE;
	task->state->esp = task->state->ebp - (5 * sizeof(uint32_t));

	// Return stack for iret. eip, cs, eflags, esp, ss.
	*(void**)task->state->esp = entry;
	*((uint32_t*)task->state->esp + 1) = GDT_SEG_CODE_PL3;
	*((uint32_t*)task->state->esp + 2) = EFLAGS_IF;
	*((uint32_t*)task->state->esp + 3) = (uint32_t)task->state->ebp;
	*((uint32_t*)task->state->esp + 4) = GDT_SEG_DATA_PL3;

	task_setup_execdata(task);
	vfs_open("/dev/stdin", O_RDONLY, task);
	vfs_open("/dev/stdout", O_WRONLY, task);
	vfs_open("/dev/stderr", O_WRONLY, task);

	char tname[10];
	snprintf(tname, 10, "task%d", task->pid);
	sysfs_add_file(tname, sfs_read, NULL, (void*)task);
	return task;
}

task_t* task_fork(task_t* to_fork, isf_t* state) {
	interrupts_disable();

	task_t* task = alloc_task();
	strcpy(task->name, to_fork->name);
	task->entry = to_fork->entry;
	task->parent = to_fork;
	task->environ = to_fork->environ;
	task->envc = to_fork->envc;
	task->argv = to_fork->argv;
	task->argc = to_fork->argc;
	memcpy(task->cwd, to_fork->cwd, TASK_PATH_MAX);

	task_add_mem(task, to_fork->stack, task->stack, STACKSIZE, VMEM_SECTION_DATA, TASK_MEM_FREE);
	memcpy(task->stack, to_fork->stack, STACKSIZE);

	task_add_mem_flat(task, task->state, PAGE_SIZE, VMEM_SECTION_DATA, TASK_MEM_FREE);
	memcpy(task->state, state, sizeof(isf_t));

	task_add_mem_flat(task, task->kernel_stack, STACKSIZE, VMEM_SECTION_KERNEL, TASK_MEM_FREE);
	memcpy(task->kernel_stack, to_fork->kernel_stack, STACKSIZE);

	// Adjust esp
	intptr_t diff = state->esp - to_fork->kernel_stack;
	task->state->esp = task->kernel_stack + diff;

	void* kernel_start = VMEM_ALIGN_DOWN(&__kernel_start);
	uint32_t kernel_size = (uint32_t)&__kernel_end - (uint32_t)kernel_start;
	task_add_mem_flat(task, kernel_start, kernel_size, VMEM_SECTION_KERNEL, 0);

	struct task_mem* alloc = to_fork->memory_allocations;
	for(; alloc; alloc = alloc->next) {
		if(alloc->flags & TASK_MEM_FORK) {
			void* phys_addr = zmalloc_a(alloc->len);
			memcpy(phys_addr, alloc->phys_addr, alloc->len);
			task_add_mem(task, alloc->virt_addr, phys_addr, alloc->len, alloc->section, alloc->flags);
		}
	}

	task_setup_execdata(task);

	memcpy(task->files, to_fork->files, sizeof(vfs_file_t) * VFS_MAX_OPENFILES);
	for(uint32_t num = 0; num < VFS_MAX_OPENFILES; num++) {
		if(task->files[num].inode) {
			task->files[num].task = task;
		}
	}

	task->state->cr3 = (uint32_t)paging_get_context(task->memory_context);
	task->state->eax = 0;
	task->state->ebx = 0;

	char tname[10];
	snprintf(tname, 10, "task%d", task->pid);
	sysfs_add_file(tname, sfs_read, NULL, (void*)task);
	return task;
}

void task_cleanup(task_t* t) {
	char tname[10];
	snprintf(tname, 10, "task%d", t->pid);
	sysfs_rm_file(tname);

	struct task_mem* alloc = t->memory_allocations;
	for(; alloc; alloc = alloc->next) {
		if(alloc->flags & TASK_MEM_FREE) {
			kfree(alloc->phys_addr);
		}
	}

	vmem_rm_context(t->memory_context);

	// FIXME No good since we pass those in as static strings occasionally
	//kfree_array(t->environ);
	//kfree_array(t->argv);

	kfree(t);
}

static size_t sfs_read(void* dest, size_t size, size_t offset, void* meta) {
	if(offset) {
		return 0;
	}

	size_t rsize = 0;
	task_t* task = (task_t*)meta;

	sysfs_printf("%-10s: %d\n", "pid", task->pid);
	sysfs_printf("%-10s: %s\n", "name", task->name);
	sysfs_printf("%-10s: 0x%x\n", "stack", task->stack);
	sysfs_printf("%-10s: 0x%x\n", "entry", task->entry);
	sysfs_printf("%-10s: 0x%x\n", "sbrk", task->sbrk);
	sysfs_printf("%-10s: %d\n", "state", task->task_state);
	sysfs_printf("%-10s: %s\n", "cwd", task->cwd);
	sysfs_printf("%-10s: %d\n", "argc", task->argc);

	sysfs_printf("%-10s: ", "argv");
	for(int i = 0; i < task->argc; i++) {
		sysfs_printf("%s ", task->argv[i]);
	}
	sysfs_printf("\n");

	sysfs_printf("%-10s: ", "environ");
	for(int i = 0; i < task->envc; i++) {
		sysfs_printf("%s ", task->environ[i]);
	}
	sysfs_printf("\n");

	sysfs_printf("\nOpen files:\n");
	for(int i = 0; i < VFS_MAX_OPENFILES; i++) {
		if(!task->files[i].inode) {
			continue;
		}

		sysfs_printf("%3d %-10s %s\n", i,
			vfs_flags_verbose(task->files[i].flags), task->files[i].path);
	}

	sysfs_printf("\nTask memory:\n");
	struct task_mem* alloc = task->memory_allocations;
	for(; alloc; alloc = alloc->next) {
		sysfs_printf("0x%-8x -> 0x%-8x length 0x%-6x %-10s\n",
			alloc->phys_addr, alloc->virt_addr, alloc->len,
			vmem_section_verbose(alloc->section));
	}


	return rsize;
}
