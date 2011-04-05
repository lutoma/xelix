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

static task_t* firstTask = NULL;
static task_t* lastTask = NULL;
static task_t* currentTask = NULL;
static uint32_t highestPid = -1;

/* Add new task to schedule. Assumes that the added task is
 * currently running and interrupts are disabled. Not to be called
 * directly, see task.c.
 */
void scheduler_add()
{
	task_t* thisTask = (task_t*)kmalloc(sizeof(task_t));
	
	thisTask->pid = ++highestPid;
	thisTask->parent = 0; // Implement me
	thisTask->next = NULL;

	if(firstTask == NULL) // First task
		firstTask = thisTask;
	else // If not first task, append to list
		lastTask->next = thisTask;

	lastTask = thisTask;

	log("scheduler: registered new task with PID %d\n", thisTask->pid);
}

task_t* scheduler_getCurrentTask()
{
	return currentTask;
}

/* Called by the PIT a few hundred times per second. For performance
 * reasons and to make sure nothing gets modified, directly gets called
 * from our assembler interrupt handler before everything else.
 */
void scheduler_select(void)
{
	// No tasks at all
	if(firstTask == NULL)
		return;
	
	if(currentTask->next == NULL)
		currentTask = firstTask;
	else
		currentTask = currentTask->next;
}
