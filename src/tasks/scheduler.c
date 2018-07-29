/* scheduler.c: Selecting which task is being executed next
 * Copyright © 2011 Lukas Martini, Fritz Grimpen
 * Copyright © 2012-2018 Lukas Martini
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

#include <lib/log.h>
#include <memory/kmalloc.h>
#include <hw/cpu.h>
#include <hw/interrupts.h>
#include <lib/panic.h>
#include <lib/string.h>
#include <memory/vmem.h>
#include <lib/multiboot.h>
#include <tasks/elf.h>

#define STACKSIZE PAGE_SIZE

task_t* current_task = NULL;
uint64_t highest_pid = -0;

void scheduler_yield()
{
	asm("int 0x31");
}

void scheduler_terminate_current()
{
	current_task->task_state = TASK_STATE_TERMINATED;
	scheduler_yield();
}

void scheduler_remove(task_t *t)
{
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

	kfree(t->stack);
	vmem_rm_context(t->memory_context);

	if(t->binary_start) {
		kfree(t->binary_start);
	}

	task_memory_allocation_t* ta = t->memory_allocations;
	while(ta) {
		kfree(ta->addr);
		task_memory_allocation_t* to_free = ta;
		ta = ta->next;
		kfree(to_free);
	}

	kfree(t->state);
	kfree_array(t->environ);
	kfree_array(t->argv);
	kfree(t);
}

/* Setup a new task, including the necessary paging context.
 * However, mapping the program itself into the context is
 * UP TO YOU as the scheduler has no clue about how long
 * your program is.
 */
task_t* scheduler_new(void* entry, task_t* parent, char name[SCHEDULER_MAXNAME],
	char** environ, uint32_t envc, char** argv, uint32_t argc,
	struct vmem_context* memory_context, bool map_structs)
{
	task_t* task = (task_t*)kmalloc_a(sizeof(task_t));

	task->stack = kmalloc_a(STACKSIZE);
	memset(task->stack, 0, STACKSIZE);

	if (map_structs) {
		// 1:1 map the stack
		vmem_rm_page_virt(memory_context, task->stack);

		struct vmem_page* page = vmem_new_page();
		page->section = VMEM_SECTION_KERNEL;
		page->cow = 0;
		page->allocated = 1;
		page->virt_addr = task->stack;
		page->phys_addr = task->stack;
		vmem_add_page(memory_context, page);
	}

	task->state = kmalloc_a(sizeof(cpu_state_t));
	task->memory_context = memory_context;

	// Stack
	task->state->esp = task->stack + STACKSIZE - 3;
	task->state->ebp = task->state->esp;

	*(task->state->ebp + 1) = (intptr_t)scheduler_terminate_current;
	*(task->state->ebp + 2) = (intptr_t)NULL; // base pointer

	// Instruction pointer (= start of the program)
	task->entry = entry;
	task->state->eip = entry;
	task->state->eflags = 0x200;
	task->state->cs = 0x08;
	task->state->ds = 0x10;
	task->state->ss = 0x10;

	task->pid = ++highest_pid;
	strcpy(task->name, name);
	task->parent = parent;
	task->task_state = TASK_STATE_RUNNING;
	task->environ = environ;
	task->envc = envc;
	task->argv = argv;
	task->argc = argc;

	if(parent)
		strncpy(task->cwd, parent->cwd, SCHEDULER_TASK_PATH_MAX);
	else
		strcpy(task->cwd, "/");

	return task;
}

// Add new task to schedule.
void scheduler_add(task_t *task)
{
	interrupts_disable();

	// No task yet?
	if(current_task == NULL)
	{
		current_task = task;
		task->next = task;
		task->previous = task;
	} else {
		task->next = current_task->next;
		task->previous = current_task;
		current_task->next = task;
	}

	interrupts_enable();
}

task_t* scheduler_get_current()
{
	return current_task;
}

// Forks a task. Returns forked task on success, NULL on error.
task_t* scheduler_fork(task_t* to_fork, cpu_state_t* state)
{
	log(LOG_INFO, "scheduler: Received fork request for %d <%s>\n", to_fork->pid, to_fork->name);

	char* __env[] = { "PS1=[$USER@$HOST $PWD]# ", "HOME=/root", "TERM=dash", "PWD=/", "USER=root", "HOST=default", NULL };
	char* __argv[] = { "dash", "-liV", NULL };

	// FIXME Make copy of memory context instead of just using the same
	task_t* new_task = scheduler_new(state->eip, to_fork, "fork", __env, 6, __argv, 2, to_fork->memory_context, false);

	// Copy registers
	new_task->state->ebx = state->ebx;
	new_task->state->ecx = state->ecx;
	new_task->state->edx = state->edx;
	new_task->state->ds = state->ds;
	new_task->state->edi = state->edi;
	new_task->state->esi = state->esi;
	new_task->state->cs = state->cs;
	new_task->state->eflags = state->eflags;
	new_task->state->ss = state->ss;

	// Copy stack & calculate correct stack offset for fork's ESP
	memcpy(new_task->stack, to_fork->stack, STACKSIZE);
	new_task->state->esp = new_task->stack + (state->esp - to_fork->stack);

	strncpy(new_task->name, to_fork->name, SCHEDULER_MAXNAME);
	return new_task;
}

// Called by the PIT a few hundred times per second.
task_t* scheduler_select(cpu_state_t* last_regs)
{
	if(unlikely(scheduler_state == SCHEDULER_INITIALIZING))
	{
		scheduler_state = SCHEDULER_INITIALIZED;
		return current_task;
	}

	// Save CPU register state of previous task
	current_task->state = last_regs;

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
			current_task->task_state == TASK_STATE_WAITING) {

			/* We're back at the original task, which is stopped or waiting.
			 * This means that every task currently in the task list is waiting,
			 * which is an unresolvable deadlock.
			 */
			if(current_task == orig_task) {
				panic("scheduler: All tasks are waiting or stopped.\n");
			}

			continue;
		}

		break;
	}

	return current_task;
}

void scheduler_init()
{
	scheduler_state = SCHEDULER_INITIALIZING;
}
