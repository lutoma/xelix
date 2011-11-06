#pragma once

/* Copyright © 2011 Lukas Martini
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
#include <memory/vm.h>

// Single linked list
typedef struct task {
	uint32_t pid;
	struct task *parent;
	cpu_state_t* state;
	struct task* next;
	struct task* last;

	struct vm_context *memory_context;

	// Current task state
	enum {
		TASK_STATE_KILLED,
		TASK_STATE_TERMINATED,
		TASK_STATE_BLOCKING,
		TASK_STATE_STOPPED,
		TASK_STATE_RUNNING
	} task_state;

	// Syscall calling convention (Linux or Unix)
	enum {
		TASK_SYSCONV_LINUX,
		TASK_SYSCONV_UNIX
	} sys_call_conv;
} task_t;

int scheduler_state;

task_t *scheduler_newTask(void *entry, task_t *parent);
void scheduler_add(task_t *task);
void scheduler_terminateCurrentTask();
task_t* scheduler_getCurrentTask();
task_t* scheduler_select(cpu_state_t* lastRegs);
void scheduler_init();
