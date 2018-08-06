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

#include <lib/generic.h>
#include <hw/cpu.h>
#include <memory/vmem.h>

#define SCHEDULER_MAXNAME 256
#define SCHEDULER_TASK_PATH_MAX 256

typedef struct task_memory_allocation {
	void* phys_addr;
	void* virt_addr;
	struct task_memory_allocation* next;
} task_memory_allocation_t;

typedef struct task {
	uint32_t pid;
	char name[SCHEDULER_MAXNAME];
	struct task* parent;
	cpu_state_t* state;
	struct task* next;
	struct task* previous;

	void* stack;
	void* entry;
	void* binary_start;
	void* sbrk;
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

	// TODO Is this actually the same as PATH_MAX in our toolchain?
	char cwd[SCHEDULER_TASK_PATH_MAX + 1];
} task_t;

enum {
	SCHEDULER_OFF,
	SCHEDULER_INITIALIZING,
	SCHEDULER_INITIALIZED
} scheduler_state;

task_t* scheduler_new(void* entry, task_t* parent, char name[SCHEDULER_MAXNAME],
	char** environ, uint32_t envc, char** argv, uint32_t argc,
	struct vmem_context* memory_context, bool map_structs);
void scheduler_add(task_t *task);
void scheduler_terminate_current();
task_t* scheduler_get_current();
task_t* scheduler_select(cpu_state_t* lastRegs);
void scheduler_init();
void scheduler_yield();
void scheduler_remove(task_t *t);
task_t* scheduler_fork(task_t* to_fork, cpu_state_t* state);
