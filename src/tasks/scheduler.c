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
#include <mem/kmalloc.h>
#include <tasks/worker.h>

static struct scheduler_qentry* current_entry = NULL;
enum scheduler_state scheduler_state;

task_t* scheduler_get_current() {
	return current_entry ? current_entry->task : NULL;
}

void scheduler_add(task_t* task) {
	struct scheduler_qentry* entry = kmalloc(sizeof(struct scheduler_qentry));
	entry->task = task;
	entry->worker = NULL;
	task->qentry = entry;

	// No entry yet?
	if(current_entry == NULL) {
		current_entry = entry;
		entry->next = entry;
		entry->prev = entry;
	} else {
		entry->next = current_entry->next;
		entry->next->prev = entry;
		entry->prev = current_entry;
		current_entry->next = entry;
	}

	if(task->ctty) {
		task->ctty->fg_task = task;
	}
}

void scheduler_add_worker(worker_t* worker) {
	struct scheduler_qentry* entry = kmalloc(sizeof(struct scheduler_qentry));
	entry->worker = worker;
	entry->task = NULL;

	// No entry yet?
	if(current_entry == NULL) {
		current_entry = entry;
		entry->next = entry;
		entry->prev = entry;
	} else {
		entry->next = current_entry->next;
		entry->next->prev = entry;
		entry->prev = current_entry;
		current_entry->next = entry;
	}
}

task_t* scheduler_find(uint32_t pid) {
	struct scheduler_qentry* start = current_entry;
	for(struct scheduler_qentry* entry = start; entry->next != start; entry = entry->next) {
		task_t* t = entry->task;
		if(!t) {
			continue;
		}

		if(t->pid == pid && t->task_state != TASK_STATE_REPLACED &&
			t->task_state != TASK_STATE_TERMINATED &&
			t->task_state != TASK_STATE_REAPED) {
			return t;
		}

		if(entry->next == entry) {
			break;
		}
	}
	return NULL;
}

void scheduler_yield() {
	int_enable();
	asm("int $0x31;");
}

static inline void unlink(struct scheduler_qentry* entry) {
	serial_printf("unlink pid %d, entry %#x\n", entry->task->pid, entry);
	if(entry->next == entry || entry->prev == entry) {
		panic("scheduler: No more queued tasks to execute (PID 1 killed?).\n");
	}
	serial_printf("1 entry is now %#x, kfreeing\n", entry);

	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;

	serial_printf("2 entry is now %#x, kfreeing\n", entry);
	if(entry->task) {
		task_cleanup(entry->task);
	}

	serial_printf("entry is now %#x, kfreeing\n", entry);
	//kfree(entry);
}

isf_t* scheduler_select(isf_t* last_regs) {
	int_disable();

	if(unlikely(scheduler_state != SCHEDULER_INITIALIZED)) {
		if(scheduler_state == SCHEDULER_INITIALIZING) {
			scheduler_state = SCHEDULER_INITIALIZED;
			goto ret;
		}

		// SCHEDULER_OFF
		return NULL;
	}

	// Save CPU register state of previous task
	if(last_regs) {
		if(current_entry->task) {
			memcpy(current_entry->task->state, last_regs, sizeof(isf_t));
		} else if(current_entry->worker) {
			memcpy(current_entry->worker->state, last_regs, sizeof(isf_t));
		}
	}

	/* Cycle through tasks until we find one that isn't terminated,
	 * while along the way unlinking the killed/terminated ones.
	*/
	current_entry = current_entry->next;

	for(;; current_entry = current_entry->next) {
		if(unlikely(current_entry == NULL)) {
			panic("scheduler: Task list corrupted (current_task->next was NULL).\n");
		}

		if(current_entry->worker) {
			if(current_entry->worker->stopped == true) {
				unlink(current_entry);
				continue;
			}

			break;
		}

		task_t* current_task = current_entry->task;
		if(current_task->task_state == TASK_STATE_TERMINATED) {
			task_userland_eol(current_task);
			continue;
		}

		if(current_task->task_state == TASK_STATE_REAPED ||
			current_task->task_state == TASK_STATE_REPLACED) {
			unlink(current_entry);
			continue;
		}

		if(current_task->task_state == TASK_STATE_STOPPED ||
			current_task->task_state == TASK_STATE_WAITING ||
			current_task->task_state == TASK_STATE_ZOMBIE) {
			continue;
		}

		break;
	}

	if(current_entry->task) {
		current_entry->task->task_state = TASK_STATE_RUNNING;
	}

ret:
	if(current_entry->task) {
		// FIXME per-task storage of SSE state needed?
		//memcpy(sse_state, new_task->state->sse_state, 512);
		gdt_set_tss(current_entry->task->kernel_stack + KERNEL_STACK_SIZE);
		return current_entry->task->state;
	} else if(current_entry->worker) {
		return current_entry->worker->state;
	}
	return NULL;
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	struct scheduler_qentry* entry = current_entry;
	size_t rsize = 0;
	sysfs_printf("# pid uid gid ppid state name memory tty\n")

	do {
		task_t* task = entry->task;
		if(!task) {
			sysfs_printf("-1 0 0 0 R \"%s\" 0 /dev/null\n", entry->worker->name);
			goto next;
		}

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
			case TASK_STATE_SLEEPING: state = 'W'; break;
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
		entry = entry->next;
	} while(entry != current_entry);

	return rsize;
}

void scheduler_init() {
	scheduler_state = SCHEDULER_INITIALIZING;
	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("tasks", &sfs_cb);
}
