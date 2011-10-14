/* scheduler.c: Selecting which task is being executed next
 * Copyright © 2011 Lukas Martini
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
#include <memory/vm.h>

#define STACKSIZE PAGE_SIZE
#define STATE_OFF 0
#define STATE_INITIALIZING 1
#define STATE_INITIALIZED 2

task_t* currentTask = NULL;
uint64_t highestPid = -1;

/* If we kill a process using scheduler_terminateCurrentTask, we also
 * fire an IRQ to switch to the next process.  However, that way, the
 * next process running would get less cpu time, as the next timer
 * interrupt happens to be faster. Therefore, if this var is set, the
 * scheduler 'skips' one tick, effectively giving the running process
 * more time.
 */
bool skipnext = 0;

void scheduler_terminateCurrentTask()
{
	log(LOG_DEBUG, "scheduler: Deleting current task\n");

	if(currentTask->next == currentTask)
		currentTask = NULL;
	else
	{
		currentTask->next->last = currentTask->last;
		currentTask->last->next = currentTask->next;
	}

	skipnext = true;
	while(true) asm("int 0x20; hlt");
}

void scheduler_remove(task_t *t)
{
	log(LOG_DEBUG, "scheduler: Deleting task %d\n", t->pid);

	t->next->last = t->last;
	t->last->next = t->next;
}

static struct vm_context *setupMemoryContext(void *stack)
{
	log(LOG_DEBUG, "scheduler: Setup new Memory Context [%d]\n", stack);
	struct vm_context *ctx = vm_new();
	
	/* Protect unused kernel space (0x7fff0000 - 0x7fffc000) */
	int addr = 0x7fff0000;
	while (addr <= 0x7fffc000)
	{
		struct vm_page *currPage = vm_new_page();
		currPage->allocated = 0;
		currPage->section = VM_SECTION_KERNEL;
		currPage->readonly = 1;
		currPage->virt_addr = (void *)addr;

		vm_add_page(ctx, currPage);

		addr += PAGE_SIZE;
	}

	struct vm_page *stackPage = vm_new_page();
	stackPage->allocated = 1;
	stackPage->section = VM_SECTION_STACK;
	stackPage->virt_addr = (void *)0x7fffe000;
	stackPage->phys_addr = stack;

	/* Additional stack space if already allocated stack is full */

	struct vm_page *stackPage2 = vm_new_page();
	stackPage2->section = VM_SECTION_STACK;
	stackPage2->virt_addr = (void *)0x7fffd000;

	vm_add_page(ctx, stackPage);
	vm_add_page(ctx, stackPage2);

	/* Map memory from 0x100000 to 0x17f000 (Kernel-related data) */
	int pos = 0x100000;
	while (pos <= 0x17f000)
	{
		struct vm_page *currPage = vm_new_page();
		currPage->allocated = 1;
		currPage->section = VM_SECTION_KERNEL;
		currPage->readonly = 1;
		currPage->virt_addr = (void *)pos;
		currPage->phys_addr = (void *)pos;

		vm_add_page(ctx, currPage);

		pos += PAGE_SIZE;
	}

	/* Map unused interrupt handler */

	struct vm_page *intHandler = vm_new_page();
	intHandler->section = VM_SECTION_KERNEL;
	intHandler->readonly = 1;
	intHandler->allocated = 1;
	intHandler->virt_addr = (void *)0x7ffff000;
	intHandler->phys_addr = (void *)0;

	vm_add_page(ctx, intHandler);

	return ctx;
}

// Add new task to schedule. task.c provides an interface to this.
void scheduler_add(void* entry)
{
	task_t* thisTask = (task_t*)kmalloc(sizeof(task_t));
	
	void* stack = kmalloc_a(STACKSIZE);
	memset(stack, 0, STACKSIZE);
	
	thisTask->state = stack + STACKSIZE - sizeof(cpu_state_t) - 3;
	thisTask->memory_context = setupMemoryContext(stack);
	
	// Stack
	thisTask->state->esp = stack + STACKSIZE - 3;
	thisTask->state->ebp = thisTask->state->esp;

	*(thisTask->state->ebp + 1) = scheduler_terminateCurrentTask;
	*(thisTask->state->ebp + 2) = NULL; // base pointer
	
	// Instruction pointer (= start of the program)
	thisTask->state->eip = entry;
	thisTask->state->eflags = 0x200;
	thisTask->state->cs = 0x08;
	thisTask->state->ds = 0x10;
	thisTask->state->ss = 0x10;

	thisTask->pid = ++highestPid;
	thisTask->parent = (currentTask == NULL) ? 0 : currentTask->pid; // Implement me
	thisTask->task_state = TASK_STATE_RUNNING;
	thisTask->sys_call_conv = TASK_SYSCONV_LINUX;

	interrupts_disable();

	// No task yet?
	if(currentTask == NULL)
	{
		currentTask = thisTask;
		thisTask->next = thisTask;
		thisTask->last = thisTask;
	} else {
		thisTask->next = currentTask->next;
		thisTask->last = currentTask;
		currentTask->next = thisTask;
	}

	interrupts_enable();
	
	log(LOG_INFO, "scheduler: Registered new task with PID %d\n", thisTask->pid);
}

task_t* scheduler_getCurrentTask()
{
	return currentTask;
}

// Called by the PIT a few hundred times per second.
task_t* scheduler_select(cpu_state_t* lastRegs)
{
	if(scheduler_state == STATE_INITIALIZING)
	{
		scheduler_state = STATE_INITIALIZED;
		return currentTask;
	}

	currentTask->state = lastRegs;

	if(skipnext == true)
	{
		skipnext = false;
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

		if (currentTask == NULL || currentTask->task_state == TASK_STATE_RUNNING)
			break;
	}

	if (currentTask == NULL)
		panic("scheduler: No tasks left - init killed?");

	return currentTask;
}

void scheduler_init()
{
	scheduler_state = STATE_INITIALIZING;
}
