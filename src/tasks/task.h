#pragma once

/* Copyright © 2011-2019 Lukas Martini
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

#include <int/int.h>
#include <fs/vfs.h>
#include <mem/vmem.h>
#include <tasks/signal.h>
#include <spinlock.h>

#define TASK_MAXNAME 256
#define TASK_PATH_MAX 256

#define TASK_MEM_FORK	0x1
#define TASK_MEM_FREE	0x2

enum task_mem_section {
	TMEM_SECTION_STACK,   /* Initial stack */
	TMEM_SECTION_CODE,    /* Contains program code and is read-only */
	TMEM_SECTION_DATA,    /* Contains static data */
	TMEM_SECTION_HEAP,    /* Allocated by brk(2) at runtime */
	TMEM_SECTION_KERNEL,  /* Contains kernel-internal data */
} section;

struct task_mem {
	struct task_mem* next;
	void* virt_addr;
	void* phys_addr;
	uint32_t len;
	enum task_mem_section section;
	int flags;
};

struct elf_load_ctx {
	void* virt_end;

	// Interpreter for dynamic linking
	char* interp;
	// Dynamic library string table (virt address)
	void* dynstrtab;

	#if 0
	// Required dependencies, as offsets to dynstrtab
	uint32_t dyndeps[MAXDEPS];
	uint32_t ndyndeps;
	#endif
};

typedef struct task {
	uint32_t pid;
	uint16_t uid;
	uint16_t gid;
	uint16_t euid;
	uint16_t egid;

	char name[TASK_MAXNAME];
	struct task* parent;
	isf_t* state;
	struct task* next;
	struct task* previous;

	void* stack;
	void* entry;
	void* sbrk;

	// Kernel stack used for interrupts. This will be loaded into the TSS.
	void* kernel_stack;

	struct vmem_context* memory_context;
	struct task_mem* memory_allocations;

	// Current task state
	enum {
		// Killed, used regardless of specific signal (SIGKILL vs SIGTERM).
		TASK_STATE_TERMINATED,

		// SIGSTOP
		TASK_STATE_STOPPED,

		/* Task has been replaced by a different task with an identical PID.
		 * This is used by our implementation of execve.
		 */
		TASK_STATE_REPLACED,
		TASK_STATE_RUNNING,

		// Task has called waitpid syscall
		TASK_STATE_WAITING,

		// Task is currently in a syscall
		TASK_STATE_SYSCALL
	} task_state;

	// Exit code in a format compatible with the waitpid() stat_loc field
	int exit_code;

	char** environ;
	char** argv;
	uint32_t argc;
	uint32_t envc;

	vfs_file_t files[VFS_MAX_OPENFILES];
	spinlock_t file_open_lock;

	struct sigaction signal_handlers[32];
	uint32_t signal_mask;

	struct {
		/* If task_state is TASK_STATE_WAITING, this specifies the task we are
		 * waiting for, or any child if 0.
		 */
		uint32_t wait_for;
		int* stat_loc;
	} wait_context;

	// TODO Is this actually the same as PATH_MAX in our toolchain?
	char cwd[TASK_PATH_MAX + 1];
	char binary_path[TASK_PATH_MAX + 1];

	/* A task-specific errno variable. After a syscall return, this will be put
	 * into the ebx register, from where the userland syscall handler will
	 * assign it to errno.
	 *
	 * Usually, this should be written to using the sc_errno macro from
	 * lib/errno.h.
	 */
	uint32_t syscall_errno;

	/* If set, this will cause the interrupt handler to not return this task's
	 * state after a syscall as usual, but instead run the scheduler as if a
	 * timer interrupt had occured. Used for scheduler_yield/to make sure tasks
	 * that called exit() don't get called again.
	 */
	bool interrupt_yield;

	// ELF loader context, needed for dlopen/dlsym.
	struct elf_load_ctx elf_ctx;
} task_t;

task_t* task_new(task_t* parent, uint32_t pid, char name[TASK_MAXNAME],
	char** environ, uint32_t envc, char** argv, uint32_t argc);
void task_set_initial_state(task_t* task);
int task_fork(task_t* to_fork, isf_t* state);
int task_execve(task_t* task, char* path, char** argv, char** env);
int task_exit(task_t* task, int code);
int task_setid(task_t* task, int which, int id);
void task_cleanup(task_t* t);
int task_chdir(task_t* task, const char* dir);
void* task_sbrk(task_t* task, int32_t length, int32_t l2);

#define task_add_mem_flat(task, start, size, section, flags) \
	task_add_mem(task, start, start, size, section, flags)
void task_add_mem(task_t* task, void* virt_start, void* phys_start,
	uint32_t size, enum task_mem_section section, int flags);


static inline char* task_mem_section_verbose(enum task_mem_section section) {
	char* names[] = {
		"Stack",   /* Initial stack */
		"Code",    /* Contains program code and is read-only */
		"Data",    /* Contains static data */
		"Heap",    /* Allocated by brk(2) at runtime */
		"Kernel",  /* Contains kernel-internal data */
	};

	if(section < ARRAY_SIZE(names)) {
		return names[section];
	}
	return NULL;
}
