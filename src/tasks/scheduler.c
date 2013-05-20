/* scheduler.c: Selecting which task is being executed next
 * Copyright © 2011 Lukas Martini, Fritz Grimpen
 * Copyright © 2013 Lukas Martini
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
#include <interrupts/interface.h>
#include <lib/panic.h>
#include <lib/string.h>
#include <memory/vmem.h>

#define STACKSIZE PAGE_SIZE
#define STATE_OFF 0
#define STATE_INITIALIZING 1
#define STATE_INITIALIZED 2

task_t* currentTask = NULL;
uint64_t highestPid = -0;

/* If we kill a process using scheduler_terminateCurrentTask, we also
 * fire an IRQ to switch to the next process.  However, that way, the
 * next process running would get less cpu time, as the next timer
 * interrupt happens to be faster. Therefore, if this var is set, the
 * scheduler 'skips' one tick, effectively giving the running process
 * more time.
 */
#define SKIP_WAIT 2
#define SKIP_NEXT 1
#define SKIP_OFF  0
int skipnext = SKIP_OFF;

void scheduler_yield()
{
	skipnext = SKIP_WAIT;
	asm("int 0x31");
}

void scheduler_terminate_current()
{
	log(LOG_DEBUG, "scheduler: Deleting current task <%s>\n", currentTask->name);

	if(currentTask->next == currentTask)
		currentTask = NULL;
	else
	{
		currentTask->next->last = currentTask->last;
		currentTask->last->next = currentTask->next;
	}

	scheduler_yield();
}

void scheduler_remove(task_t *t)
{
	log(LOG_DEBUG, "scheduler: Deleting task %d <%s>\n", t->pid, t->name);

	t->next->last = t->last;
	t->last->next = t->next;
}

/* Setup a new task, including the necessary paging context.
 * However, mapping the program itself into the context is
 * UP TO YOU as the scheduler has no clue about how long
 * your program is.
 */
task_t* scheduler_new(void* entry, task_t* parent, char name[SCHEDULER_MAXNAME],
	char** environ, char** argv, int argc, struct vmem_context* memory_context)
{
	task_t* thisTask = (task_t*)kmalloc_a(sizeof(task_t));
	
	void* stack = kmalloc_a(STACKSIZE);
	memset(stack, 0, STACKSIZE);

	// 1:1 map the stack
	vmem_rm_page_virt(memory_context, stack);

	struct vmem_page* page = vmem_new_page();
	page->section = VMEM_SECTION_KERNEL;
	page->cow = 0;
	page->allocated = 1;
	page->virt_addr = stack;
	page->phys_addr = stack;
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
	
	thisTask->state = stack + STACKSIZE - sizeof(cpu_state_t) - 3;
	thisTask->memory_context = memory_context;

	// Stack
	thisTask->state->esp = stack + STACKSIZE - 3;
	thisTask->state->ebp = thisTask->state->esp;

	*(thisTask->state->ebp + 1) = (intptr_t)scheduler_terminate_current;
	*(thisTask->state->ebp + 2) = NULL; // base pointer
	
	// Instruction pointer (= start of the program)
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
		task->last = task;
	} else {
		task->next = currentTask->next;
		task->last = currentTask;
		currentTask->next = task;
	}

	interrupts_enable();
	
	log(LOG_INFO, "scheduler: Registered new task with PID %d <%s>\n", task->pid, task->name);
}

task_t* scheduler_get_current()
{
	return currentTask;
}

// Called by the PIT a few hundred times per second.
task_t* scheduler_select(cpu_state_t* lastRegs)
{
	if(unlikely(scheduler_state == STATE_INITIALIZING))
	{
		scheduler_state = STATE_INITIALIZED;
		return currentTask;
	}

	currentTask->state = lastRegs;

	if(skipnext == SKIP_WAIT) skipnext = SKIP_NEXT;
	else if(skipnext == SKIP_NEXT)
	{
		skipnext = SKIP_OFF;
		return currentTask;
	}

	while (1)
	{
		currentTask = currentTask->next;
		
		if (currentTask->task_state == TASK_STATE_KILLED ||
				currentTask->task_state == TASK_STATE_TERMINATED)
		{
			if (currentTask->next == currentTask)
				currentTask->next = NULL;
			scheduler_remove(currentTask);
		}

		if (unlikely(currentTask == NULL || currentTask->task_state == TASK_STATE_RUNNING))
			break;
	}

	return currentTask;
}

void scheduler_init()
{
	scheduler_state = STATE_INITIALIZING;
}
