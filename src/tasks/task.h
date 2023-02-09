#pragma once

/* Copyright © 2011-2020 Lukas Martini
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
#include <mem/valloc.h>
#include <tasks/signal.h>
#include <tty/term.h>

// Should be kept in sync with value in boot/*-boot.S
#define KERNEL_STACK_PAGES 4
#define KERNEL_STACK_SIZE PAGE_SIZE * KERNEL_STACK_PAGES

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

	char name[VFS_NAME_MAX];
	struct task* parent;
	struct scheduler_qentry* qentry;
	struct valloc_ctx vmem;
	isf_t* state;
	void* entry;
	void* sbrk;
	void* stack;
	size_t stack_size;

	// Kernel stack used for interrupts. This will be loaded into the TSS.
	void* kernel_stack;

	// Controlling terminal
	struct term* ctty;

	// Current task state
	enum {
		/* Killed/exited, used regardless of specific signal/exit reason.
		 * Tasks will only be in this state briefly: After task termination,
		 * but before the scheduler has called task_userland_eol. Once that has
		 * happened, the task switches to TASK_STATE_ZOMBIE.
		 */
		TASK_STATE_TERMINATED,

		/*
		 * Task has been killed and task_userland_eol has run, but the parent
		 * process hasn't run waitpid yet. Once that happens, the task switches
		 * to TASK_STATE_REAPED and will be deallocated.
		 */
		TASK_STATE_ZOMBIE,

		/* Terminated task, parent has run waitpid. Should be ignored when
		 * iterating the task list and will be removed by the scheduler
		 * at its next invocation.
		 */
		TASK_STATE_REAPED,

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

	// These arrays are not necessarily NULL-terminated, always check argc/envc!
	char** environ;
	char** argv;
	uint32_t argc;
	uint32_t envc;

	vfs_file_t files[CONFIG_VFS_MAX_OPENFILES];

	struct sigaction signal_handlers[32];
	uint32_t signal_mask;

	struct {
		/* If task_state is TASK_STATE_WAITING, this specifies the task we are
		 * waiting for, or any child if 0.
		 */
		uint32_t wait_for;

		// Used to pass result pid from wait_finish to task_waitpid
		int wait_res_pid;
		int* stat_loc;
	} wait_context;

	char cwd[VFS_PATH_MAX + 1];
	char binary_path[VFS_PATH_MAX + 1];

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

	struct task* strace_observer;
	int strace_fd;

	struct sysfs_file* sysfs_file;
} task_t;

task_t* task_new(task_t* parent, uint32_t pid, char name[VFS_NAME_MAX],
	char** environ, uint32_t envc, char** argv, uint32_t argc);
void task_set_initial_state(task_t* task);
int task_fork(task_t* to_fork, isf_t* state);
int task_execve(task_t* task, char* path, char** argv, char** env);
int task_exit(task_t* task, int code);
int task_setid(task_t* task, int which, int id);
void task_userland_eol(task_t* t);
void task_cleanup(task_t* t);
int task_chdir(task_t* task, const char* dir);
int task_strace(task_t* task, isf_t* state);

#include <tasks/mem.h>
