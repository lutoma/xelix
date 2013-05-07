/* scheduler.c: Selecting which task is being executed next
 * Copyright © 2011 Lukas Martini, Fritz Grimpen
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

void scheduler_terminateCurrentTask()
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

static struct vmem_context *setupMemoryContext(void *stack)
{
	log(LOG_DEBUG, "scheduler: Setup new Memory Context [%d]\n", stack);
	struct vmem_context *ctx = vmem_new();
	
	/* Protect unused kernel space (0x7fff0000 - 0x7fffc000) */
	for(int addr = 0x7fff0000; addr <= 0x7fffc000; addr += PAGE_SIZE)
	{
		struct vmem_page *currPage = vmem_new_page();
		currPage->allocated = 0;
		currPage->section = VMEM_SECTION_KERNEL;
		currPage->readonly = 1;
		currPage->virt_addr = (void *)addr;

		vmem_add_page(ctx, currPage);
	}

	struct vmem_page *stackPage = vmem_new_page();
	stackPage->allocated = 1;
	stackPage->section = VMEM_SECTION_STACK;
	stackPage->virt_addr = (void *)0x7fffe000;
	stackPage->phys_addr = stack;

	/* Additional stack space if already allocated stack is full */

	struct vmem_page *stackPage2 = vmem_new_page();
	stackPage2->section = VMEM_SECTION_STACK;
	stackPage2->virt_addr = (void *)0x7fffd000;

	vmem_add_page(ctx, stackPage);
	vmem_add_page(ctx, stackPage2);

	/* Map memory from 0x100000 to 0x17f000 (Kernel-related data) */
	int pos = 0x100000;
	while (pos <= 0x17f000)
	{
		struct vmem_page *currPage = vmem_new_page();
		currPage->allocated = 1;
		currPage->section = VMEM_SECTION_KERNEL;
		currPage->readonly = 1;
		currPage->virt_addr = (void *)pos;
		currPage->phys_addr = (void *)pos;

		vmem_add_page(ctx, currPage);

		pos += PAGE_SIZE;
	}

	/* Map unused interrupt handler */

	struct vmem_page *intHandler = vmem_new_page();
	intHandler->section = VMEM_SECTION_KERNEL;
	intHandler->readonly = 1;
	intHandler->allocated = 1;
	intHandler->virt_addr = (void *)0x7ffff000;
	intHandler->phys_addr = (void *)0;

	vmem_add_page(ctx, intHandler);

	return ctx;
}


/* Setup a new task, including the necessary paging context.
 * However, mapping the program itself into the context is
 * UP TO YOU as the scheduler has no clue about how long
 * your program is.
 */
task_t *scheduler_newTask(void *entry, task_t *parent, char name[SCHEDULER_MAXNAME], char** environ, char** argv, int argc)
{
	task_t* thisTask = (task_t*)kmalloc(sizeof(task_t));
	
	void* stack = kmalloc_a(STACKSIZE);
	memset(stack, 0, STACKSIZE);
	
	thisTask->state = stack + STACKSIZE - sizeof(cpu_state_t) - 3;
	thisTask->memory_context = setupMemoryContext(stack);
	thisTask->memory_context = vmem_kernelContext;

	// Stack
	thisTask->state->esp = stack + STACKSIZE - 3;
	thisTask->state->ebp = thisTask->state->esp;

	*(thisTask->state->ebp + 1) = (intptr_t)scheduler_terminateCurrentTask;
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

task_t* scheduler_getCurrentTask()
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
