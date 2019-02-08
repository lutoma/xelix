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

#include <hw/interrupts.h>
#include <fs/vfs.h>
#include <memory/vmem.h>
#include <tasks/signal.h>
#include <spinlock.h>

#define TASK_MAXNAME 256
#define TASK_PATH_MAX 256

#define TASK_MEM_FORK	0x1
#define TASK_MEM_FREE	0x2

struct task_mem {
	struct task_mem* next;
	void* virt_addr;
	void* phys_addr;
	uint32_t len;
	enum vmem_section section;
	int flags;
};

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
	struct task_mem* memory_allocations;

	// Current task state
	enum {
		TASK_STATE_TERMINATED,
		TASK_STATE_STOPPED,
		TASK_STATE_REPLACED,
		TASK_STATE_RUNNING,
		TASK_STATE_WAITING,
		TASK_STATE_SYSCALL
	} task_state;

	char** environ;
	char** argv;
	uint32_t argc;
	uint32_t envc;

	vfs_file_t files[VFS_MAX_OPENFILES];
	spinlock_t file_open_lock;

	struct sigaction signal_handlers[32];
	uint32_t signal_mask;

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

task_t* task_new(task_t* parent, uint32_t pid, char name[TASK_MAXNAME],
	char** environ, uint32_t envc, char** argv, uint32_t argc);
void task_set_initial_state(task_t* task, void* entry);
task_t* task_fork(task_t* to_fork, isf_t* state);
void task_reset(task_t* task, task_t* parent, char name[TASK_MAXNAME],
	char** environ, uint32_t envc, char** argv, uint32_t argc);
void task_cleanup(task_t* t);

#define task_add_mem_flat(task, start, size, section, flags) \
	task_add_mem(task, start, start, size, section, flags)

void task_add_mem(task_t* task, void* virt_start, void* phys_start,
	uint32_t size, enum vmem_section section, int flags);
