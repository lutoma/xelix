/* task.c: Userland tasks
 * Copyright © 2011-2018 Lukas Martini
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

#define STACKSIZE PAGE_SIZE

uint32_t highest_pid = 0;
extern void* __kernel_start;
extern void* __kernel_end;

static size_t sfs_read(void* dest, size_t size, void* meta) {
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

	sysfs_printf("%-10s: %d\n", "envc", task->argc);
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

		sysfs_printf("%3d %s\n", i, task->files[i].path);
	}
	return rsize;
}

/* Sets up a new task, including the necessary paging context, stacks,
 * interrupt stack frame etc. The binary still has to be mapped into the paging
 * context separately (usually in the ELF loader).
 */
task_t* task_new(void* entry, task_t* parent, char name[TASK_MAXNAME],
	char** environ, uint32_t envc, char** argv, uint32_t argc) {

	task_t* task = (task_t*)kmalloc(sizeof(task_t));
	bzero(task, sizeof(task_t));

	// tmalloc automatically zeros memory.
	task->state = tmalloc_a(sizeof(isf_t), task);
	task->stack = tmalloc_a(STACKSIZE, task);
	task->kernel_stack = tmalloc_a(STACKSIZE, task);
	task->memory_context = vmem_new();
	vmem_set_task(task->memory_context, task);

	strcpy(task->name, name);
	task->entry = entry;
	task->pid = ++highest_pid;
	task->parent = parent;
	task->task_state = TASK_STATE_RUNNING;
	task->environ = environ;
	task->envc = envc;
	task->argv = argv;
	task->argc = argc;
	task->interrupt_yield = false;

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
	vmem_map_flat(task->memory_context, task->stack, STACKSIZE, VMEM_SECTION_DATA);
	vmem_map_flat(task->memory_context, task->kernel_stack, STACKSIZE, VMEM_SECTION_KERNEL);
	vmem_map_flat(task->memory_context, task->state, sizeof(task->state), VMEM_SECTION_DATA);

	void* kernel_start = VMEM_ALIGN_DOWN(&__kernel_start);
	uint32_t kernel_size = (uint32_t)&__kernel_end - (uint32_t)kernel_start;
	vmem_map_flat(task->memory_context, kernel_start, kernel_size, VMEM_SECTION_KERNEL);

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


// Forks a task. Returns forked task on success, NULL on error.
task_t* task_fork(task_t* to_fork, isf_t* state)
{
	log(LOG_INFO, "scheduler: Received fork request for %d <%s>\n", to_fork->pid, to_fork->name);

	char* __env[] = { "PS1=[$USER@$HOST $PWD]# ", "HOME=/root", "TERM=dash", "PWD=/", "USER=root", "HOST=default", NULL };
	char* __argv[] = { "dash", "-liV", NULL };

	// FIXME Make copy of memory context instead of just using the same
	task_t* new_task = task_new(0, to_fork, "fork", __env, 6, __argv, 2);

	// Copy registers
	new_task->state->ebx = state->ebx;
	new_task->state->ecx = state->ecx;
	new_task->state->edx = state->edx;
	new_task->state->ds = state->ds;
	new_task->state->edi = state->edi;
	new_task->state->esi = state->esi;
//	new_task->state->cs = state->cs;
//	new_task->state->eflags = state->eflags;

	// Copy stack & calculate correct stack offset for fork's ESP
	memcpy(new_task->stack, to_fork->stack, STACKSIZE);
	new_task->state->esp = new_task->stack + (state->esp - to_fork->stack);

	strncpy(new_task->name, to_fork->name, TASK_MAXNAME);
	return new_task;
}

void task_cleanup(task_t* t) {
	char tname[10];
	snprintf(tname, 10, "task%d", t->pid);
	sysfs_rm_file(tname);

/*
	vmem_rm_context(t->memory_context);

	task_memory_allocation_t* ta = t->memory_allocations;
	while(ta) {
		kfree(ta->addr);
		task_memory_allocation_t* to_free = ta;
		ta = ta->next;
		kfree(to_free);
	}

	kfree_array(t->environ);
	kfree_array(t->argv);
	kfree(t);
*/
}
