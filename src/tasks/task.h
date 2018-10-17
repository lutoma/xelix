#pragma once

/* Copyright © 2011-2018 Lukas Martini
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

#include <hw/interrupts.h>
#include <fs/vfs.h>

#define TASK_MAXNAME 256
#define TASK_PATH_MAX 256

typedef struct task_memory_allocation {
	void* addr;
	struct task_memory_allocation* next;
} task_memory_allocation_t;

typedef struct task {
	uint32_t pid;
	char name[TASK_MAXNAME];
	struct task* parent;
	isf_t* state;
	struct task* next;
	struct task* previous;

	void* stack;
	void* entry;
	void* sbrk;

	// Kernel stack used for interrupts. This will be loaded into the TSS
	void* kernel_stack;

	struct vmem_context* memory_context;
	task_memory_allocation_t* memory_allocations;

	// Current task state
	enum {
		TASK_STATE_KILLED,
		TASK_STATE_TERMINATED,
		TASK_STATE_BLOCKING,
		TASK_STATE_STOPPED,
		TASK_STATE_RUNNING,
		TASK_STATE_WAITING,
		TASK_STATE_SYSCALL
	} task_state;

	char** environ;
	char** argv;
	uint32_t argc;
	uint32_t envc;

	vfs_file_t files[VFS_MAX_OPENFILES];

	// TODO Is this actually the same as PATH_MAX in our toolchain?
	char cwd[TASK_PATH_MAX + 1];
	uint32_t syscall_errno;

	/* If set, this will cause the interrupt handler to not return this task's
	 * state after a syscall as usual, but instead run the scheduler as if a
	 * timer interrupt had occured. Used for scheduler_yield/to make sure tasks
	 * that called exit() don't get called again.
	 */
	bool interrupt_yield;
} task_t;

task_t* task_new(void* entry, task_t* parent, char name[TASK_MAXNAME],
	char** environ, uint32_t envc, char** argv, uint32_t argc);
task_t* task_fork(task_t* to_fork, isf_t* state);
void task_cleanup(task_t* t);
