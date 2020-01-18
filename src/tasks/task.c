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
#include <tasks/wait.h>
#include <tasks/elf.h>
#include <mem/vmem.h>
#include <mem/kmalloc.h>
#include <mem/palloc.h>
#include <mem/vmem.h>
#include <mem/i386-gdt.h>
#include <int/int.h>
#include <tty/tty.h>
#include <tty/ioctl.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <fs/pipe.h>
#include <string.h>
#include <errno.h>

// Should be kept in sync with value in boot/*-boot.S
#define KERNEL_STACK_SIZE PAGE_SIZE * 4

static uint32_t highest_pid = 0;

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size);

static task_t* alloc_task(task_t* parent, uint32_t pid, char name[VFS_NAME_MAX],
	char** environ, uint32_t envc, char** argv, uint32_t argc) {

	task_t* task = zmalloc(sizeof(task_t));
	task->vmem_ctx = zmalloc(sizeof(struct vmem_context));
	task->state = palloc(1);
	bzero(task->state, sizeof(isf_t));

	task->kernel_stack = palloc(4);

	task_add_mem_flat(task, task->state, PAGE_SIZE,
		TMEM_SECTION_KERNEL, TASK_MEM_FREE | TASK_MEM_PALLOC);

	task_add_mem_flat(task, task->kernel_stack, KERNEL_STACK_SIZE,
		TMEM_SECTION_KERNEL, TASK_MEM_FREE | TASK_MEM_PALLOC);

	// FIXME Should have TMEM_SECTION_KERNEL, but that would break task_sigjmp_crt0
	task_add_mem_flat(task, KERNEL_START, KERNEL_SIZE + 0x5000,
		TMEM_SECTION_CODE, 0);

	task->pid = pid ? pid : __sync_add_and_fetch(&highest_pid, 1);
	task->task_state = TASK_STATE_RUNNING;
	task->interrupt_yield = false;

	strcpy(task->name, name);
	memcpy(task->cwd, parent ? parent->cwd : "/", VFS_PATH_MAX);
	task->parent = parent;

	task->envc = envc;
	task->argc = argc;
	task->environ = kmalloc(sizeof(char*) * task->envc);
	task->argv = kmalloc(sizeof(char*) * task->argc);

	for(int i = 0; i < task->envc; i++) {
		task->environ[i] = strdup(environ[i]);
	}

	for(int i = 0; i < task->argc; i++) {
		task->argv[i] = strdup(argv[i]);
	}

	char tname[10];
	snprintf(tname, 10, "task%d", task->pid);
	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};

	task->sysfs_file = sysfs_add_file(tname, &sfs_cb);
	task->sysfs_file->meta = (void*)task;
	return task;
}

/* Sets up a new task, including the necessary paging context, stacks,
 * interrupt stack frame etc. The binary still has to be mapped into the paging
 * context separately (usually in the ELF loader).
 */
task_t* task_new(task_t* parent, uint32_t pid, char name[VFS_NAME_MAX],
	char** environ, uint32_t envc, char** argv, uint32_t argc) {

	task_t* task = alloc_task(parent, pid, name, environ, envc, argv, argc);

	// Allocate initial stack. Will dynamically grow, so be conservative.
	task->stack_size = PAGE_SIZE * 2;
	task->stack = zpalloc(task->stack_size / PAGE_SIZE);
	task_add_mem(task, (void*)TASK_STACK_LOCATION - task->stack_size, task->stack,
		task->stack_size, TMEM_SECTION_STACK,
		TASK_MEM_FREE | TASK_MEM_PALLOC | TASK_MEM_FORK);

	vfs_open(task, "/dev/stdin", O_RDONLY);
	vfs_open(task, "/dev/stdout", O_WRONLY);
	vfs_open(task, "/dev/stderr", O_WRONLY);
	return task;
}


void task_set_initial_state(task_t* task) {
	task_setup_execdata(task);

	task->state->ds = GDT_SEG_DATA_PL3;
	task->state->cr3 = (uint32_t)vmem_get_hwdata(task->vmem_ctx);
	task->state->ebp = 0;
	task->state->esp = (void*)TASK_STACK_LOCATION - sizeof(iret_t);

	// Return stack for iret
	iret_t* iret = task->stack + task->stack_size - sizeof(iret_t);
	iret->eip = task->entry;
	iret->cs = GDT_SEG_CODE_PL3;
	iret->eflags = EFLAGS_IF;
	iret->user_esp = TASK_STACK_LOCATION;
	iret->ss = GDT_SEG_DATA_PL3;
}

/* Called by the scheduler whenever a task terminates from the userland
 * perspective. For example, this is called when a task changes to
 * TASK_STATE_TERMINATED, but not for TASK_STATE_REPLACED, since that task
 * lives on from the userland POV.
 */
void task_userland_eol(task_t* t) {
	t->task_state = TASK_STATE_ZOMBIE;

	task_t* init = scheduler_find(1);
	for(task_t* i = t->next; i->next != t->next; i = i->next) {
		if(i->parent == t) {
			i->parent = init;
		}
	}

	if(t->parent) {
		if(t->parent->task_state == TASK_STATE_WAITING) {
			wait_finish(t->parent, t);
		}
		task_signal(t->parent, t, SIGCHLD, t->parent->state);
	}

	if(t->ctty && t == t->ctty->fg_task) {
		t->ctty->fg_task = t->parent;
	}

	if(t->strace_observer && t->strace_fd) {
		vfs_close(t->strace_observer, t->strace_fd);
	}
}

/* Called by scheduler whenever it encounters a task with TASK_STATE_REAPED or
 * TASK_STATE_REPLACED. Should deallocate all task objects, but be transparent
 * to userspace.
 */
void task_cleanup(task_t* t) {
	// Could have already been removed by execve
	if(t->sysfs_file) {
		sysfs_rm_file(t->sysfs_file);
	}

	struct task_mem* alloc = t->mem_allocs;
	while(alloc) {
		if(alloc->flags & TASK_MEM_FREE) {
			if(alloc->flags & TASK_MEM_PALLOC) {
				pfree((uintptr_t)alloc->phys_addr / PAGE_SIZE, alloc->len / PAGE_SIZE);
			} else {
				kfree(alloc->phys_addr);
			}
		}

		struct task_mem* old_alloc = alloc;
		alloc = alloc->next;
		kfree(old_alloc);
	}

	t->mem_allocs = NULL;
	vmem_rm_context(t->vmem_ctx);

	kfree_array(t->environ, t->envc);
	kfree_array(t->argv, t->argc);
	kfree(t);
}

static task_t* _fork(task_t* to_fork, isf_t* state) {
	task_t* task = alloc_task(to_fork, 0, to_fork->name, to_fork->environ,
		to_fork->envc, to_fork->argv, to_fork->argc);

	memcpy(task->cwd, to_fork->cwd, VFS_PATH_MAX);
	memcpy(task->state, state, sizeof(isf_t));
	memcpy(task->kernel_stack, to_fork->kernel_stack, KERNEL_STACK_SIZE);
	memcpy(task->binary_path, to_fork->binary_path, sizeof(task->binary_path));
	memcpy(task->files, to_fork->files, sizeof(vfs_file_t) * VFS_MAX_OPENFILES);

	task->uid = to_fork->uid;
	task->gid = to_fork->gid;
	task->euid = to_fork->euid;
	task->egid = to_fork->egid;
	task->ctty = to_fork->ctty;
	task->stack_size = to_fork->stack_size;
	task->sbrk = to_fork->sbrk;

	task_setup_execdata(task);

	// Adjust kernel esp
	intptr_t diff = state->esp - to_fork->kernel_stack;
	task->state->esp = task->kernel_stack + diff;

	struct task_mem* alloc = to_fork->mem_allocs;
	for(; alloc; alloc = alloc->next) {
		if(alloc->flags & TASK_MEM_FORK) {
			void* phys_addr;

			if(alloc->flags & TASK_MEM_PALLOC) {
				phys_addr = zpalloc(alloc->len / PAGE_SIZE);
			} else {
				phys_addr = zmalloc_a(alloc->len);
			}
			memcpy(phys_addr, alloc->phys_addr, alloc->len);
			task_add_mem(task, alloc->virt_addr, phys_addr, alloc->len, alloc->section, alloc->flags);
		}
	}

	task->state->cr3 = (uint32_t)vmem_get_hwdata(task->vmem_ctx);

	/* Set syscall return values for the forked task – need to set here since
	 * the regular syscall return handling only affects the main process.
	 */
	task->state->eax = 0;
	task->state->ebx = 0;

	scheduler_add(task);
	return task;
}

int task_fork(task_t* to_fork, isf_t* state) {
	task_t* task = _fork(to_fork, state);
	if(task) {
		return task->pid;
	} else {
		return -1;
	}
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
	char** __argv = task_copy_strings(task, argv, &__argc);
	char** __env = task_copy_strings(task, env, &__envc);
	if(!__argv || !__env) {
		log(LOG_WARN, "execve: array check fail\n");
		return 0;
	}

	/* Normally removed in task_cleanup, but it may take until after this
	 * function is done for the scheduler to invoke it. Since task_new adds a
	 * new sysfs file, remove the old one here to avoid conflicts.
	 */
	if(task->sysfs_file) {
		sysfs_rm_file(task->sysfs_file);
	}
	task->sysfs_file = NULL;

	task_t* new_task = task_new(task->parent, task->pid, path, __env, __envc, __argv, __argc);
	kfree_array(__argv, __argc);
	kfree_array(__env, __envc);

	memcpy(new_task->cwd, task->cwd, VFS_PATH_MAX);
	new_task->uid = task->uid;
	new_task->gid = task->gid;
	new_task->euid = task->euid;
	new_task->egid = task->egid;
	new_task->strace_observer = task->strace_observer;
	new_task->strace_fd = task->strace_fd;
	new_task->ctty = task->ctty;

	if(elf_load_file(new_task, path) == -1) {
		return -1;
	}

	for(int i = 0; i < VFS_MAX_OPENFILES; i++) {
		struct vfs_file* file = &task->files[i];
		if(file->refs && !(file->flags & O_CLOEXEC)) {
			memcpy(&new_task->files[i], file, sizeof(struct vfs_file));
		}
	}

	scheduler_add(new_task);
	task->task_state = TASK_STATE_REPLACED;
	task->interrupt_yield = true;
	return 0;
}

int task_chdir(task_t* task, const char* dir) {
	if(vfs_access(task, dir, R_OK | X_OK) < 0) {
		return -1;
	}

	int fd = vfs_open(task, dir, O_RDONLY);
	if(fd == -1) {
		return -1;
	}

	vfs_stat_t* stat = kmalloc(sizeof(vfs_stat_t));
	if(vfs_fstat(task, fd, stat) != 0) {
		kfree(stat);
		vfs_close(task, fd);
		sc_errno = ENOENT;
		return -1;
	}

	if(vfs_mode_to_filetype(stat->st_mode) != FT_IFDIR) {
		vfs_close(task, fd);
		sc_errno = ENOTDIR;
		return -1;
	}

	kfree(stat);
	strcpy(task->cwd, vfs_get_from_id(fd, task)->path);
	vfs_close(task, fd);
	return 0;
}

int task_strace(task_t* task, isf_t* state) {
	task_t* fork = _fork(task, state);
	if(!fork) {
		return -1;
	}

	int pipe[2];
	if(vfs_pipe(task, pipe) != 0) {
		return -1;
	}

	fork->strace_observer = task;
	fork->strace_fd = pipe[1];
	return pipe[0];
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	size_t rsize = 0;
	task_t* task = (task_t*)ctx->fp->meta;

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
	sysfs_printf("%-10s: %s\n", "tty", task->ctty ? task->ctty->path : "");
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
	struct task_mem* alloc = task->mem_allocs;
	for(; alloc; alloc = alloc->next) {
		sysfs_printf("0x%-8x -> 0x%-8x length 0x%-6x %-10s (%salloc)\n",
			alloc->virt_addr, alloc->phys_addr, alloc->len,
			task_mem_section_verbose(alloc->section),
			alloc->flags & TASK_MEM_PALLOC ? "p" : "km");
	}


	return rsize;
}
