/* task.c: Userland tasks
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

#include <tasks/task.h>
#include <tasks/execdata.h>
#include <tasks/syscall.h>
#include <tasks/wait.h>
#include <tasks/elf.h>
#include <mem/kmalloc.h>
#include <mem/mem.h>
#include <mem/vm.h>
#include <mem/i386-gdt.h>
#include <int/int.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <fs/pipe.h>
#include <string.h>
#include <errno.h>

static uint32_t highest_pid = 0;
static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size);

static task_t* alloc_task(task_t* parent, uint32_t pid, char name[VFS_NAME_MAX],
	char** environ, uint32_t envc, char** argv, uint32_t argc) {

	task_t* task = zmalloc(sizeof(task_t));
	vm_new(&task->vmem, NULL);

	/* Map parts of the kernel marked as UL_VISIBLE into the task address
	 * space (But readable only to PL0). These are the functions and data
	 * structures used in the interrupt handler before the paging context is
	 * switched.
	 */
	if(!vm_alloc_at(&task->vmem, NULL, RDIV(UL_VISIBLE_SIZE, PAGE_SIZE),
		UL_VISIBLE_START, UL_VISIBLE_START, VM_FIXED)) {

		return NULL;
	}

	task->pid = pid ? pid : __sync_add_and_fetch(&highest_pid, 1);
	task->task_state = TASK_STATE_RUNNING;
	task->interrupt_yield = false;

	strlcpy(task->name, name, VFS_NAME_MAX);
	if(parent) {
		memcpy(task->cwd, parent->cwd, VFS_PATH_MAX);
	} else {
		task->cwd[0] = '/';
	}

	task->parent = parent;
	task->envc = envc;
	task->argc = argc;
	task->environ = kmalloc(sizeof(char*) * task->envc);
	task->argv = kmalloc(sizeof(char*) * task->argc);

	if(!task->environ || !task->argv) {
		return NULL;
	}

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

static inline int map_task(task_t* task) {
	vm_alloc_t vmem1;
	vm_alloc_t vmem2;
	vm_alloc_t* mvmem[] = {&vmem1, &vmem2};

	int flags[] = {VM_RW, VM_FREE};
	struct vm_ctx* ctx[] = {VM_KERNEL, &task->vmem};

	if(!vm_alloc_many(2, ctx, mvmem, 1, NULL, flags)) {
		kfree(task);
		return -1;
	}

	task->state = vmem1.addr;
	bzero(task->state, sizeof(isf_t));

	// Kernel stack used during interrupts while this task is running
	if(!vm_alloc_many(2, ctx, mvmem, KERNEL_STACK_PAGES, NULL, flags)) {
		vm_free(&vmem1);
		vm_free(&vmem2);
		kfree(task);
		return -1;
	}

	task->kernel_stack = vmem1.addr;
	return 0;
}

/* Sets up a new task, including the necessary paging context, stacks,
 * interrupt stack frame etc. The binary still has to be mapped into the paging
 * context separately (usually in the ELF loader).
 */
task_t* task_new(task_t* parent, uint32_t pid, char name[VFS_NAME_MAX],
	char** environ, uint32_t envc, char** argv, uint32_t argc) {

	task_t* task = alloc_task(parent, pid, name, environ, envc, argv, argc);
	if(!task) {
		return NULL;
	}

	if(map_task(task) != 0) {
		return NULL;
	}

	// Allocate initial stack. Will dynamically grow, so be conservative.
	task->stack_size = PAGE_SIZE * 2;

	if(!vm_alloc_at(&task->vmem, NULL, 2, (void*)TASK_STACK_LOCATION - task->stack_size, NULL,
		VM_USER | VM_RW | VM_FREE | VM_TFORK | VM_FIXED)) {
		return NULL;
	}

	vfs_open(task, "/dev/stdin", O_RDONLY);
	vfs_open(task, "/dev/stdout", O_WRONLY);
	vfs_open(task, "/dev/stderr", O_WRONLY);
	return task;
}

void task_set_initial_state(task_t* task) {
	task_setup_execdata(task);

	task->state->ds = GDT_SEG_DATA_PL3;
	task->state->cr3 = (uint32_t)vm_pagedir(&task->vmem);
	task->state->ebp = 0;
	task->state->esp = (void*)TASK_STACK_LOCATION - sizeof(iret_t);

	// Temporarily map part of the userland stack into kernel memory to set up
	// stack for initial iret
	vm_alloc_t alloc;
	iret_t* iret = vm_map(VM_KERNEL, &alloc, &task->vmem, task->state->esp, sizeof(iret_t), 0);

	iret->eip = task->entry;
	iret->cs = GDT_SEG_CODE_PL3;
	iret->eflags = EFLAGS_IF;
	iret->user_esp = (void*)TASK_STACK_LOCATION;
	iret->ss = GDT_SEG_DATA_PL3;

	vm_free(&alloc);
}

/* Called by the scheduler whenever a task terminates from the userland
 * perspective. For example, this is called when a task changes to
 * TASK_STATE_TERMINATED, but not for TASK_STATE_REPLACED, since that task
 * lives on from the userland POV.
 */
void task_userland_eol(task_t* t) {
	t->task_state = TASK_STATE_ZOMBIE;

	task_t* init = scheduler_find(1);
	for(struct scheduler_qentry* e = t->qentry->next; e->next != t->qentry->next; e = e->next) {
		if(e->task && e->task->parent == t) {
			e->task->parent = init;
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

	task_free(t);
}

static task_t* _fork(task_t* to_fork, isf_t* state) {
	task_t* task = alloc_task(to_fork, 0, to_fork->name, to_fork->environ,
		to_fork->envc, to_fork->argv, to_fork->argc);

	task->uid = to_fork->uid;
	task->gid = to_fork->gid;
	task->euid = to_fork->euid;
	task->egid = to_fork->egid;
	task->ctty = to_fork->ctty;
	task->stack_size = to_fork->stack_size;
	task->sbrk = to_fork->sbrk;

	memcpy(task->cwd, to_fork->cwd, VFS_PATH_MAX);
	memcpy(task->binary_path, to_fork->binary_path, sizeof(task->binary_path));
	memcpy(task->files, to_fork->files, sizeof(vfs_file_t) * CONFIG_VFS_MAX_OPENFILES);

	if(vm_clone(&task->vmem, &to_fork->vmem) != 0) {
		return NULL;
	}

	// FIXME transfer potentially updated environ
	task_setup_execdata(task);

	/* Allocate task state and kernel stack. It's important this is done after
	 * the vm_clone above, since it could otherwise end up in a memory region
	 * that was already occupied in the forked task.
	 */
	if(map_task(task) != 0) {
		return NULL;
	}

	memcpy(task->state, state, sizeof(isf_t));
	memcpy(task->kernel_stack, to_fork->kernel_stack, KERNEL_STACK_SIZE);

	// Adjust kernel esp
	intptr_t diff = state->esp - to_fork->kernel_stack;
	task->state->esp = task->kernel_stack + diff;

	task->state->cr3 = (uint32_t)vm_pagedir(&task->vmem);

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

	for(int i = 0; i < CONFIG_VFS_MAX_OPENFILES; i++) {
		struct vfs_file* file = &task->files[i];

		// FIXME flags seem to get mangled during fork/execve
		//if(file->refs && !(file->flags & O_CLOEXEC)) {
		if(file->refs) {
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
	sysfs_printf("%-10s: %p\n", "entry", task->entry);
	sysfs_printf("%-10s: %p\n", "sbrk", task->sbrk);
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
	for(int i = 0; i < CONFIG_VFS_MAX_OPENFILES; i++) {
		if(!task->files[i].inode) {
			continue;
		}

		sysfs_printf("%3d %-10s %s\n", i,
			vfs_flags_verbose(task->files[i].flags), task->files[i].path);
	}

	sysfs_printf("\nTask memory:\n");
	vm_alloc_t* range = task->vmem.ranges;
	for(; range; range = range->next) {
		sysfs_printf("%p - %p  ->  %p - %p length %#-6lx\n",
			range->addr, range->addr + range->size - 1,
			range->phys, range->phys + range->size - 1,
			range->size);
	}


	return rsize;
}
