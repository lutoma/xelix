/* scheduler.c: Userland task scheduling
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

#include "scheduler.h"
#include <panic.h>
#include <fs/sysfs.h>
#include <int/int.h>

static task_t* current_task = NULL;
enum scheduler_state scheduler_state;

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
		task->next->previous = task;
		task->previous = current_task;
		current_task->next = task;
	}

	if(task->ctty) {
		task->ctty->fg_task = task;
	}
}

task_t* scheduler_find(uint32_t pid) {
	task_t* start = current_task;
	for(task_t* t = start; t->next != start; t = t->next) {
		if(t->pid == pid && t->task_state != TASK_STATE_REPLACED &&
			t->task_state != TASK_STATE_TERMINATED &&
			t->task_state != TASK_STATE_REAPED) {
			return t;
		}

		if(t->next == t) {
			break;
		}
	}
	return NULL;
}

static inline void unlink(task_t *t) {
	if(t->next == t || t->previous == t) {
		panic("scheduler: No more queued tasks to execute (PID 1 killed?).\n");
	}

	t->next->previous = t->previous;
	t->previous->next = t->next;
	task_cleanup(t);
}

task_t* scheduler_select(isf_t* last_regs) {
	int_disable();
	if(unlikely(scheduler_state != SCHEDULER_INITIALIZED)) {
		if(scheduler_state == SCHEDULER_INITIALIZING) {
			scheduler_state = SCHEDULER_INITIALIZED;
			return current_task;
		}

		// SCHEDULER_OFF
		return NULL;
	}

	// Save CPU register state of previous task
	if(last_regs) {
		memcpy(current_task->state, last_regs, sizeof(isf_t));
	}

	/* Cycle through tasks until we find one that isn't terminated,
	 * while along the way unlinking the killed/terminated ones.
	*/
	current_task = current_task->next;

	for(;; current_task = current_task->next) {
		if(unlikely(current_task == NULL)) {
			panic("scheduler: Task list corrupted (current_task->next was NULL).\n");
		}

		if(current_task->task_state == TASK_STATE_TERMINATED) {
			task_userland_eol(current_task);
			continue;
		}

		if(current_task->task_state == TASK_STATE_REAPED ||
			current_task->task_state == TASK_STATE_REPLACED) {
			unlink(current_task);
			continue;
		}

		if(current_task->task_state == TASK_STATE_STOPPED ||
			current_task->task_state == TASK_STATE_WAITING ||
			current_task->task_state == TASK_STATE_ZOMBIE) {
			continue;
		}

		break;
	}

	return current_task;
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	task_t* task = ctx->task;
	size_t rsize = 0;
	sysfs_printf("# pid uid gid ppid state name memory tty\n")

	do {
		if(task->task_state == TASK_STATE_REPLACED) {
			goto next;
		}

		uint32_t ppid = task->parent ? task->parent->pid : 0;

		char state = '?';
		switch(task->task_state) {
			case TASK_STATE_TERMINATED: state = 'T'; break;
			case TASK_STATE_STOPPED: state = 'S'; break;
			case TASK_STATE_RUNNING: state = 'R'; break;
			case TASK_STATE_WAITING: state = 'W'; break;
			case TASK_STATE_SYSCALL: state = 'C'; break;
			default: state = 'U'; break;
		}

		vmem_t* range = task->vmem.ranges;
		uint32_t mem_alloc = 0;
		for(; range; range = range->next) {
			if(range->flags & VM_TFORK) {
				mem_alloc += range->size;
			}
		}

		sysfs_printf("%d %d %d %d %c \"%s", task->pid, task->euid, task->gid,
			ppid, state, task->name);

		for(int i = 1; i < task->argc; i++) {
			sysfs_printf(" %s", task->argv[i]);
		}
		sysfs_printf("\" %d %s\n", mem_alloc, task->ctty ? task->ctty->path : "-");

	next:
		task = task->next;
	} while(task != current_task);

	return rsize;
}

void scheduler_init() {
	scheduler_state = SCHEDULER_INITIALIZING;
	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("tasks", &sfs_cb);
}
