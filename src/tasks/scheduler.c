/* scheduler.c: Userland task scheduling
 * Copyright © 2011-2023 Lukas Martini
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
#include <mem/i386-gdt.h>
#include <tasks/worker.h>

static struct scheduler_qentry* current_entry = NULL;
struct scheduler_qentry idle_qentry;
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
	if(entry->next == entry || entry->prev == entry) {
		panic("scheduler: No more queued tasks to execute (PID 1 killed?).\n");
	}

	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;

	if(entry->task) {
		task_cleanup(entry->task);
	}

	// FIXME Free entry
	//kfree(entry);
}

static inline struct scheduler_qentry* find_runnable_qentry(struct scheduler_qentry* start) {
	struct scheduler_qentry* qe = start->next;
	struct scheduler_qentry* orig = qe;
	bool initial = true;

	for(;; qe = qe->next) {
		if(unlikely(qe == NULL)) {
			panic("scheduler: qentry list corrupted (current_entry->next was NULL).\n");
		}

		if(qe == orig && !initial) {
			return NULL;
		}

		if(initial) {
			initial = false;
		}

		if(qe->worker) {
			if(qe->worker->stopped == true) {
				unlink(qe);
				continue;
			}

			break;
		} else if(qe->task) {
			task_t* task = qe->task;
			if(task->task_state == TASK_STATE_TERMINATED) {
				task_userland_eol(task);
				continue;
			}

			if(task->task_state == TASK_STATE_REAPED ||
				task->task_state == TASK_STATE_REPLACED) {
				unlink(qe);
				continue;
			}

			if(task->task_state == TASK_STATE_STOPPED ||
				task->task_state == TASK_STATE_WAITING ||
				task->task_state == TASK_STATE_ZOMBIE) {
				continue;
			}

			if(task->task_state == TASK_STATE_SLEEPING) {
			}

			if(task->task_state == TASK_STATE_SLEEPING &&
				timer_get_tick() < task->sleep_until) {
				continue;
			}
		}

		break;
	}

	return qe;
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

	struct scheduler_qentry* qe = find_runnable_qentry(current_entry);
	if(qe) {
		current_entry = qe;
	} else {
		idle_qentry.next = current_entry->next;
		idle_qentry.prev = current_entry;
		current_entry = &idle_qentry;
	}

ret:
	if(current_entry->task) {
		current_entry->task->task_state = TASK_STATE_RUNNING;

		// FIXME per-task storage of SSE state needed?
		//memcpy(sse_state, new_task->state->sse_state, 512);
		gdt_set_tss(current_entry->task->kernel_stack + KERNEL_STACK_SIZE);
		return current_entry->task->state;
	} else if(current_entry->worker) {
		// FIXME TSS for workers?
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

		vm_alloc_t* range = task->vmem.ranges;
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

static void __attribute__((fastcall, noreturn)) do_idle(worker_t* worker) {
			int_enable();
		while(true) {
			asm("hlt;");
		}
}

void scheduler_init() {
	worker_t* idle_worker = worker_new("kidle", &do_idle);
	idle_qentry.task = NULL;
	idle_qentry.worker = idle_worker;

	scheduler_state = SCHEDULER_INITIALIZING;
	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("tasks", &sfs_cb);
}
