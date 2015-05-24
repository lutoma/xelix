/* scheduler.c: Selecting which task is being executed next
 * Copyright © 2011 Lukas Martini, Fritz Grimpen
 * Copyright © 2012-2014 Lukas Martini
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
#include <arch/i386/lib/multiboot.h>
#include <tasks/elf.h>

#define STACKSIZE PAGE_SIZE
#define STATE_OFF 0
#define STATE_INITIALIZING 1
#define STATE_INITIALIZED 2

task_t* currentTask = NULL;
uint64_t highestPid = -0;

void scheduler_yield()
{
	asm("int 0x31");
}

void scheduler_terminate_current()
{
	log(LOG_DEBUG, "scheduler: Terminating current task %d <%s>\n", currentTask->pid, currentTask->name);
	currentTask->task_state = TASK_STATE_TERMINATED;
	scheduler_yield();
}

void scheduler_remove(task_t *t)
{
	if(t->next == t || t->previous == t) {
		panic("scheduler: No more queued tasks to execute (PID 1 killed?).\n");
	}

	t->next->previous = t->previous;
	t->previous->next = t->next;
}

/* Setup a new task, including the necessary paging context.
 * However, mapping the program itself into the context is
 * UP TO YOU as the scheduler has no clue about how long
 * your program is.
 */
task_t* scheduler_new(void* entry, task_t* parent, char name[SCHEDULER_MAXNAME],
	char** environ, char** argv, int argc, struct vmem_context* memory_context, bool map_structs)
{
	task_t* thisTask = (task_t*)kmalloc_a(sizeof(task_t));
	
	thisTask->stack = kmalloc_a(STACKSIZE);
	memset(thisTask->stack, 0, STACKSIZE);

	if (map_structs) {
		// 1:1 map the stack
		vmem_rm_page_virt(memory_context, thisTask->stack);

		struct vmem_page* page = vmem_new_page();
		page->section = VMEM_SECTION_KERNEL;
		page->cow = 0;
		page->allocated = 1;
		page->virt_addr = thisTask->stack;
		page->phys_addr = thisTask->stack;
		vmem_add_page(memory_context, page);

		/* Also 1:1 map the own task_t* struct. This is kind of a braindead idea
		 * security-wise, but it works as a quick hack to let tasks get their argv,
		 * argc & environ … Should be fixed in the getexecdata syscall. FIXME
		 */
		vmem_rm_page_virt(memory_context, thisTask);

		page = vmem_new_page();
		page->section = VMEM_SECTION_KERNEL;
		page->cow = 0;
		page->allocated = 1;
		page->virt_addr = thisTask;
		page->phys_addr = thisTask;
		vmem_add_page(memory_context, page);
	}
	
	thisTask->state = kmalloc_a(sizeof(cpu_state_t));
	thisTask->memory_context = memory_context;

	// Stack
	thisTask->state->esp = thisTask->stack + STACKSIZE - 3;
	thisTask->state->ebp = thisTask->state->esp;

	*(thisTask->state->ebp + 1) = (intptr_t)scheduler_terminate_current;
	*(thisTask->state->ebp + 2) = (intptr_t)NULL; // base pointer
	
	// Instruction pointer (= start of the program)
	thisTask->entry = entry;
	thisTask->state->eip = entry;
	thisTask->state->eflags = 0x200;
	thisTask->state->cs = 0x08;
	thisTask->state->ds = 0x10;
	thisTask->state->ss = 0x10;

	thisTask->pid = ++highestPid;
	strcpy(thisTask->name, name);
	thisTask->parent = parent;
	thisTask->task_state = TASK_STATE_RUNNING;
	thisTask->environ = environ;
	thisTask->argv = argv;
	thisTask->argc = argc;

	if(parent)
		strncpy(thisTask->cwd, parent->cwd, SCHEDULER_TASK_PATH_MAX);
	else
		strcpy(thisTask->cwd, "/");

	return thisTask;
}

// Add new task to schedule.
void scheduler_add(task_t *task)
{
	interrupts_disable();

	// No task yet?
	if(currentTask == NULL)
	{
		currentTask = task;
		task->next = task;
		task->previous = task;
	} else {
		task->next = currentTask->next;
		task->previous = currentTask;
		currentTask->next = task;
	}

	interrupts_enable();
	
	log(LOG_INFO, "scheduler: Registered new task with PID %d <%s>\n", task->pid, task->name);
}

task_t* scheduler_get_current()
{
	return currentTask;
}

// Forks a task. Returns forked task on success, NULL on error.
task_t* scheduler_fork(task_t* to_fork, cpu_state_t* state)
{
	log(LOG_INFO, "scheduler: Received fork request for %d <%s>\n", to_fork->pid, to_fork->name);

	char* __env[] = { "PS1=[$USER@$HOST $PWD]# ", "HOME=/root", "TERM=dash", "PWD=/", "USER=root", "HOST=default", NULL }; 
	char* __argv[] = { "dash", "-liV", NULL };

	task_t* new_task = scheduler_new(state->eip, to_fork, "fork", __env, __argv, 2, vmem_kernelContext, false);

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
task_t* scheduler_select(cpu_state_t* lastRegs)
{
	if(unlikely(scheduler_state == STATE_INITIALIZING))
	{
		scheduler_state = STATE_INITIALIZED;
		return currentTask;
	}

	// Save CPU register state of previous task
	currentTask->state = lastRegs;

	/* Cycle through tasks until we find one that isn't killed or terminated,
	 * while along the way unlinking the killed/terminated ones.
	*/
	currentTask = currentTask->next;

	while (currentTask->task_state == TASK_STATE_KILLED ||
	       currentTask->task_state == TASK_STATE_TERMINATED)
	{
		scheduler_remove(currentTask);
		currentTask = currentTask->next;
	}

	if (unlikely(currentTask == NULL))
		panic("scheduler: No more tasks.\n");

	return currentTask;
}

void scheduler_init()
{
	scheduler_state = STATE_INITIALIZING;
}
