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

#include "dispatcher.h"
#include <lib/log.h>
#include <memory/kmalloc.h>
#include <hw/cpu.h>

/* You most probably don't want to touch this one. Uncommenting this
 * line will get you many many debug lines. And by many I mean 'about
 * 1000 per second'.
 */
//#define DEBUG
#define STACKSIZE 4096

#ifdef DEBUG
	#define intDebug(args...) log(args);
#else /* DEBUG */
	#define intDebug
#endif /* DEBUG */

task_t* firstTask = NULL;
task_t* currentTask = NULL;
task_t* lastTask = NULL;
uint64_t highestPid = -1;

// Add new task to schedule. task.c provides an interface to this.
void scheduler_add(void* entry)
{
	task_t* thisTask = (task_t*)kmalloc(sizeof(task_t));
	
	thisTask->pid = ++highestPid;
	thisTask->parent = 0; // Implement me
	thisTask->next = NULL;
	
	thisTask->state = (cpu_state_t*)kmalloc(sizeof(cpu_state_t));
	memset(thisTask->state, 0, sizeof(cpu_state_t));
	
	// Stack
	thisTask->state->esp = (uint32_t)kmalloc(STACKSIZE) + STACKSIZE;
	// Instruction pointer (= start of the program)
	thisTask->state->eip = (uint32_t)entry;
	thisTask->state->eflags = 0x200;
	thisTask->state->cs = 0x08;

	// Now add this task to our task list. A lock would be nice here.
	if(firstTask == NULL || lastTask == NULL)
		// This is the first task
		firstTask = thisTask;
	else
		// Obviously not the first task, append to list
		lastTask->next = thisTask;
	
	lastTask = thisTask;
		
	log("scheduler: Registered new task with PID %d\n", thisTask->pid);
}

task_t* scheduler_getCurrentTask()
{
	return currentTask;
}

// Called by the PIT a few hundred times per second.
task_t* scheduler_select(cpu_state_t* lastRegs)
{
		intDebug("scheduler: scheduling...\n");
		// No task at all
		if(firstTask == NULL)
			return (task_t*)NULL;
		
		// We have at least one task
		task_t* nextTask = NULL;
		
		intDebug("scheduler: Got at least one task...\n");
		
		// Is there a task currently running?
		if(currentTask != NULL)
		{
			intDebug("scheduler: currentTask at 0x%x\n",
					(int)currentTask);
			
			currentTask->state = lastRegs; // Save it's state.
			
			// Look for next task
			if(currentTask->next != NULL)
				nextTask = currentTask->next;
		}
		
		// No nextTask set yet? Take the first one
		if(nextTask == NULL)
			nextTask = firstTask;
		
		currentTask = nextTask;
		intDebug("scheduler: returning task at 0x%x with state at 0x%x \
				(esp 0x%x)\n", (int)nextTask, (int)nextTask->state,
				nextTask->state->esp);
		
		return nextTask;
}
