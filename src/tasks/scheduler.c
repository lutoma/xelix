/* scheduler.c: Userland task scheduling
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

#include "scheduler.h"
#include <log.h>
#include <memory/kmalloc.h>
#include <hw/cpu.h>
#include <hw/interrupts.h>
#include <panic.h>
#include <string.h>
#include <memory/vmem.h>
#include <memory/paging.h>
#include <multiboot.h>
#include <tasks/elf.h>
#include <fs/sysfs.h>

static task_t* current_task = NULL;

task_t* scheduler_get_current() {
	return current_task;
}

void scheduler_add(task_t *task) {
	// No task yet?
	if(current_task == NULL) {
		current_task = task;
		task->next = task;
		task->previous = task;
	} else {
		task->next = current_task->next;
		task->previous = current_task;
		current_task->next = task;
	}
}

void scheduler_remove(task_t *t) {
	if(t->next == t || t->previous == t) {
		panic("scheduler: No more queued tasks to execute (PID 1 killed?).\n");
	}

	t->next->previous = t->previous;
	t->previous->next = t->next;

	// Stop child tasks
	for(task_t* i = t->next; i->next != t->next; i = i->next) {
		if(i->parent == t) {
			scheduler_remove(i);
		}
	}

	if(t->parent && t->parent->task_state == TASK_STATE_WAITING) {
		t->parent->task_state = TASK_STATE_RUNNING;
	}

	//task_cleanup(t);
}

task_t* scheduler_select(cpu_state_t* last_regs) {
	if(unlikely(scheduler_state == SCHEDULER_INITIALIZING))
	{
		scheduler_state = SCHEDULER_INITIALIZED;
		return current_task;
	}

	// Save CPU register state of previous task
	memcpy(current_task->state, last_regs, sizeof(cpu_state_t));

	/* Cycle through tasks until we find one that isn't killed or terminated,
	 * while along the way unlinking the killed/terminated ones.
	*/
	task_t* orig_task = current_task;
	current_task = current_task->next;

	for(;; current_task = current_task->next) {
		if (unlikely(current_task == NULL)) {
			panic("scheduler: Task list corrupted (current_task->next was NULL).\n");
		}

		if(current_task->task_state == TASK_STATE_KILLED ||
			current_task->task_state == TASK_STATE_TERMINATED) {
			scheduler_remove(current_task);
			continue;
		}

		if(current_task->task_state == TASK_STATE_STOPPED ||
			current_task->task_state == TASK_STATE_WAITING ||
			current_task->task_state == TASK_STATE_SYSCALL) {

			/* We're back at the original task, which is stopped or waiting.
			 * This means that every task currently in the task list is waiting,
			 * which is an unresolvable deadlock.
			 */
			if(current_task == orig_task) {
				if(current_task->task_state == TASK_STATE_SYSCALL) {
					break;
				}

				panic("scheduler: All tasks are waiting or stopped.\n");
			}

			continue;
		}

		break;
	}

	return current_task;
}

static size_t sfs_read(void* dest, size_t size) {
	size_t rsize = 0;
	sysfs_printf("# pid ppid state name entry sbrk stack\n")

	task_t* task = current_task;
	do {
		uint32_t ppid = task->parent ? task->parent->pid : 0;

		char state = '?';
		switch(task->task_state) {
			case TASK_STATE_KILLED: state = 'K'; break;
			case TASK_STATE_TERMINATED: state = 'T'; break;
			case TASK_STATE_BLOCKING: state = 'B'; break;
			case TASK_STATE_STOPPED: state = 'S'; break;
			case TASK_STATE_RUNNING: state = 'R'; break;
			case TASK_STATE_WAITING: state = 'W'; break;
			case TASK_STATE_SYSCALL: state = 'C'; break;
		}

		sysfs_printf("%d %d %c %s 0x%x 0x%x 0x%x\n", task->pid, ppid, state, task->name, task->entry, task->sbrk, task->stack);
		task = task->next;
	} while(task != current_task);

	return rsize;
}

void scheduler_init() {
	scheduler_state = SCHEDULER_INITIALIZING;
	sysfs_add_file("tasks", sfs_read, NULL);
}
