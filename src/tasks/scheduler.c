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

#define STACKSIZE 4096
#define STATE_OFF 0
#define STATE_INITIALIZING 1
#define STATE_INITIALIZED 2

task_t* currentTask = NULL;
uint64_t highestPid = -1;

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
	
	while(true) asm("hlt");
}

void scheduler_remove(task_t *t)
{
	log(LOG_DEBUG, "scheduler: Deleting task %d\n", t->pid);

	t->next->last = t->last;
	t->last->next = t->next;
}

// Add new task to schedule. task.c provides an interface to this.
void scheduler_add(void* entry)
{
	task_t* thisTask = (task_t*)kmalloc(sizeof(task_t));
	
	void* stack = kmalloc(STACKSIZE);
	memset(stack, 0, STACKSIZE);
	
	thisTask->state = stack + STACKSIZE - sizeof(cpu_state_t) - 3;
	
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
	if(currentTask == NULL || currentTask->next == NULL)
		panic("scheduler: No tasks left - init killed?");

	if(scheduler_state == STATE_INITIALIZING)
	{
		scheduler_state = STATE_INITIALIZED;
		return currentTask;
	}

	currentTask->state = lastRegs;
	do
	{
		currentTask = currentTask->next;
		if (currentTask->task_state == TASK_STATE_KILLED ||
				currentTask->task_state == TASK_STATE_TERMINATED)
			scheduler_remove(currentTask);
	}
	while (currentTask->task_state != TASK_STATE_RUNNING);

	return currentTask;
}

void scheduler_init()
{
	scheduler_state = STATE_INITIALIZING;
}
