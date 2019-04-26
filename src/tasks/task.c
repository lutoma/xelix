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
#include <tasks/syscall.h>
#include <tasks/elf.h>
#include <mem/vmem.h>
#include <mem/kmalloc.h>
#include <mem/paging.h>
#include <mem/gdt.h>
#include <hw/interrupts.h>
#include <tty/tty.h>
#include <tty/ioctl.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <string.h>
#include <errno.h>

#define STACKSIZE PAGE_SIZE * 128
#define STACK_LOCATION 0x8000

uint32_t highest_pid = 0;

static size_t sfs_read(void* dest, size_t size, size_t offset, void* meta);

static task_t* alloc_task(task_t* parent, uint32_t pid, char name[TASK_MAXNAME],
	char** environ, uint32_t envc, char** argv, uint32_t argc) {

	task_t* task = (task_t*)zmalloc(sizeof(task_t));
	task->pid = pid ? pid : ++highest_pid;
	task->task_state = TASK_STATE_RUNNING;
	task->interrupt_yield = false;

	// State gets mapped into user memory, so needs to be one full page.
	task->state = zmalloc_a(PAGE_SIZE);
	task->stack = zmalloc_a(STACKSIZE);
	task->kernel_stack = zmalloc_a(STACKSIZE);
	task->memory_context = vmem_new();
	vmem_set_task(task->memory_context, task);

	strcpy(task->name, name);
	memcpy(task->cwd, parent ? parent->cwd : "/", TASK_PATH_MAX);
	task->parent = parent;
	task->environ = environ;
	task->envc = envc;
	task->argv = argv;
	task->argc = argc;

	char tname[10];
	snprintf(tname, 10, "task%d", task->pid);
	struct sysfs_file* file = sysfs_add_file(tname, sfs_read, NULL);
	file->meta = (void*)task;
	return task;
}

void task_add_mem(task_t* task, void* virt_start, void* phys_start,
	uint32_t size, enum task_mem_section section, int flags) {
	bool user = section != TMEM_SECTION_KERNEL;
	bool ro = section == TMEM_SECTION_CODE;
	if(section) {
		vmem_map(task->memory_context, virt_start, phys_start, size, user, ro);
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

static void map_memory(task_t* task) {
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
	task_add_mem(task, (void*)STACK_LOCATION, task->stack, STACKSIZE, TMEM_SECTION_DATA, 0);
	task_add_mem_flat(task, task->kernel_stack, STACKSIZE, TMEM_SECTION_KERNEL, 0);
	task_add_mem_flat(task, task->state, PAGE_SIZE, TMEM_SECTION_DATA, 0);

	// FIXME Should have TMEM_SECTION_KERNEL, but that would break task_sigjmp_crt0
	task_add_mem_flat(task, KERNEL_START, KERNEL_SIZE + 0x5000, TMEM_SECTION_CODE, 0);
}

/* Sets up a new task, including the necessary paging context, stacks,
 * interrupt stack frame etc. The binary still has to be mapped into the paging
 * context separately (usually in the ELF loader).
 */
task_t* task_new(task_t* parent, uint32_t pid, char name[TASK_MAXNAME],
	char** environ, uint32_t envc, char** argv, uint32_t argc) {

	task_t* task = alloc_task(parent, pid, name, environ, envc, argv, argc);
	map_memory(task);
	vfs_open("/dev/stdin", O_RDONLY, task);
	vfs_open("/dev/stdout", O_WRONLY, task);
	vfs_open("/dev/stderr", O_WRONLY, task);

	/* Set direct callbacks to console. Not technically necessary since
	 * /dev/stdout etc will also resolve through the vfs (to sysfs), but this
	 * avoids the lookup overhead.
	 */
	task->files[0].callbacks.read = tty_vfs_read;
	task->files[1].callbacks.write = tty_vfs_write;
	task->files[2].callbacks.write = tty_vfs_write;
	task->files[0].callbacks.ioctl = tty_ioctl;
	task->files[1].callbacks.ioctl = tty_ioctl;
	task->files[2].callbacks.ioctl = tty_ioctl;
	term->fg_task = task;
	return task;
}

int task_fork(task_t* to_fork, isf_t* state) {
	task_t* task = alloc_task(to_fork, 0, to_fork->name, to_fork->environ,
		to_fork->envc, to_fork->argv, to_fork->argc);

	memcpy(task->cwd, to_fork->cwd, TASK_PATH_MAX);
	memcpy(task->stack, to_fork->stack, STACKSIZE);
	memcpy(task->state, state, sizeof(isf_t));
	memcpy(task->kernel_stack, to_fork->kernel_stack, STACKSIZE);
	memcpy(task->binary_path, to_fork->binary_path, sizeof(task->binary_path));
	task->uid = to_fork->uid;
	task->gid = to_fork->gid;
	task->euid = to_fork->euid;
	task->egid = to_fork->egid;
	map_memory(task);
	task_setup_execdata(task);

	// Adjust kernel esp
	intptr_t diff = state->esp - to_fork->kernel_stack;
	task->state->esp = task->kernel_stack + diff;

	struct task_mem* alloc = to_fork->memory_allocations;
	for(; alloc; alloc = alloc->next) {
		if(alloc->flags & TASK_MEM_FORK) {
			void* phys_addr = zmalloc_a(alloc->len);
			memcpy(phys_addr, alloc->phys_addr, alloc->len);
			task_add_mem(task, alloc->virt_addr, phys_addr, alloc->len, alloc->section, alloc->flags);
		}
	}

	memcpy(task->files, to_fork->files, sizeof(vfs_file_t) * VFS_MAX_OPENFILES);
	for(uint32_t num = 0; num < VFS_MAX_OPENFILES; num++) {
		if(task->files[num].inode) {
			task->files[num].task = task;
		}
	}

	#ifdef __i386__
	task->state->cr3 = (uint32_t)paging_get_context(task->memory_context);
	#endif

	/* Set syscall return values for the forked task – need to set here since
	 * the regular syscall return handling only affects the main process.
	 */
	task->state->eax = 0;
	task->state->ebx = 0;

	scheduler_add(task);
	return task->pid;
}

int task_exit(task_t* task, int code) {
	task->task_state = TASK_STATE_TERMINATED;
	task->exit_code = code << 8;
	task->interrupt_yield = true;
	return 0;
}

// Task setuid/setgid
int task_setid(task_t* task, int which, int id) {
	if(task->euid != 0) {
		sc_errno = EPERM;
		return -1;
	}

	switch(which) {
		case 0:
			task->uid = id;
			task->euid = id;
			return 0;
		case 1:
			task->gid = id;
			task->egid = id;
			return 0;
	}
	sc_errno = EINVAL;
	return -1;
}

int task_execve(task_t* task, char* path, char** argv, char** env) {
	uint32_t __argc = 0;
	uint32_t __envc = 0;
	char** __argv = syscall_copy_array(task, argv, &__argc);
	char** __env = syscall_copy_array(task, env, &__envc);
	if(!__argv || !__env) {
		log(LOG_WARN, "execve: array check fail\n");
		return 0;
	}

	task_t* new_task = task_new(task->parent, task->pid, path, __env, __envc, __argv, __argc);
	memcpy(new_task->cwd, task->cwd, TASK_PATH_MAX);
	new_task->uid = task->uid;
	new_task->gid = task->gid;
	new_task->euid = task->euid;
	new_task->egid = task->egid;
	if(elf_load_file(new_task, path) == -1) {
		return -1;
	}

	memcpy(new_task->files, task->files, sizeof(new_task->files));
	scheduler_add(new_task);
	task->task_state = TASK_STATE_REPLACED;
	task->interrupt_yield = true;
	return 0;
}

static void clean_memory(task_t* t) {
	struct task_mem* alloc = t->memory_allocations;
	while(alloc) {
		if(alloc->flags & TASK_MEM_FREE) {
			kfree(alloc->phys_addr);
		}

		struct task_mem* old_alloc = alloc;
		alloc = alloc->next;
		kfree(old_alloc);
	}

	t->memory_allocations = NULL;
	vmem_rm_context(t->memory_context);
}

void task_set_initial_state(task_t* task) {
	task_setup_execdata(task);

	task->state->ds = GDT_SEG_DATA_PL3;
	#ifdef __i386__
	task->state->cr3 = (uint32_t)paging_get_context(task->memory_context);
	#endif
	task->state->ebp = (void*)STACK_LOCATION + STACKSIZE;
	task->state->esp = task->state->ebp - sizeof(iret_t);

	// Return stack for iret
	iret_t* iret = task->stack + STACKSIZE - sizeof(iret_t);
	iret->entry = task->entry;
	iret->cs = GDT_SEG_CODE_PL3;
	iret->eflags = EFLAGS_IF;
	iret->user_esp = (uint32_t)task->state->ebp;
	iret->ss = GDT_SEG_DATA_PL3;
}

void task_cleanup(task_t* t) {
	char tname[10];
	snprintf(tname, 10, "task%d", t->pid);
	sysfs_rm_file(tname);

	clean_memory(t);
	kfree(t->state);
	kfree(t->stack);
	kfree(t->kernel_stack);

	// FIXME No good since we pass those in as static strings occasionally
	//kfree_array(t->environ);
	//kfree_array(t->argv);

	kfree(t);
}

int task_chdir(task_t* task, const char* dir) {
	if(vfs_access(dir, R_OK | X_OK, task) < 0) {
		return -1;
	}

	int fd = vfs_open(dir, O_RDONLY, task);
	if(fd == -1) {
		return -1;
	}

	vfs_stat_t* stat = kmalloc(sizeof(vfs_stat_t));
	if(vfs_fstat(fd, stat, task) != 0) {
		kfree(stat);
		vfs_close(fd, task);
		sc_errno = ENOENT;
		return -1;
	}

	if(vfs_mode_to_filetype(stat->st_mode) != FT_IFDIR) {
		vfs_close(fd, task);
		sc_errno = ENOTDIR;
		return -1;
	}

	kfree(stat);
	strcpy(task->cwd, vfs_get_from_id(fd, task)->path);
	vfs_close(fd, task);
	return 0;
}

void* task_sbrk(task_t* task, int32_t length, int32_t l2) {
	// Legacy support: Length used to be passed in second parameter
	length = length ? length : l2;

	if(length <= 0) {
		return task->sbrk;
	}

	length = VMEM_ALIGN(length);
	void* phys_addr = zmalloc_a(length);

	if(!phys_addr) {
		sc_errno = EAGAIN;
		return (void*)-1;
	}

	// FIXME sbrk is not set properly in elf.c (?)
	void* virt_addr = task->sbrk;
	task->sbrk += length;

	task_add_mem(task, virt_addr, phys_addr, length, TMEM_SECTION_HEAP,
		TASK_MEM_FORK | TASK_MEM_FREE);

	return virt_addr;
}

static size_t sfs_read(void* dest, size_t size, size_t offset, void* meta) {
	if(offset) {
		return 0;
	}

	size_t rsize = 0;
	task_t* task = (task_t*)meta;

	sysfs_printf("%-10s: %d\n", "pid", task->pid);
	sysfs_printf("%-10s: %d\n", "uid", task->uid);
	sysfs_printf("%-10s: %d\n", "euid", task->euid);
	sysfs_printf("%-10s: %d\n", "gid", task->gid);
	sysfs_printf("%-10s: %d\n", "egid", task->egid);
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
			task_mem_section_verbose(alloc->section));
	}


	return rsize;
}
