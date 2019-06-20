/* wait.c: waitpid/wait/wait3
 * Copyright © 2019 Lukas Martini
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

#include <tasks/task.h>
#include <tasks/wait.h>
#include <errno.h>

int task_waitpid(task_t* task, int32_t child_pid, int* stat_loc, int options) {
	if(child_pid > 0) {
		task_t* target_task = scheduler_find(child_pid);
		if(!target_task || target_task->parent != task) {
			sc_errno = ECHILD;
			return -1;
		}
	} else {
		// Check if task has any children to wait for.
		bool have_children = false;
		for(task_t* i = task->next; i->next != task->next; i = i->next) {
			if(i->parent == task) {
				have_children = true;
				break;
			}
		}

		if(!have_children) {
			sc_errno = ECHILD;
			return -1;
		}

		child_pid = 0;
	}

	task->wait_context.wait_for = child_pid;
	task->wait_context.stat_loc = stat_loc;
	task->task_state = TASK_STATE_WAITING;
	task->interrupt_yield = true;

	// Will be set in wait_finish below
	task->wait_context.wait_res_pid = 0;

	// Wait until wait_finish is called
	while((volatile int)task->task_state == TASK_STATE_WAITING) {
		// FIXME Should scheduler yield immediately instead of waiting for tick
		halt();
	}
	return (volatile int)task->wait_context.wait_res_pid;
}

/* Called by the scheduler whenever a task with a parent that is in the
 * TASK_STATE_WAITING state is unlinked.
 */
void wait_finish(task_t* task, task_t* child) {
	// Check if this is the child we're waiting for
	if(task->wait_context.wait_for && task->wait_context.wait_for != child->pid) {
		return;
	}

	// This is used as return value for waitpid above.
	task->wait_context.wait_res_pid = child->pid;
	if(task->wait_context.stat_loc) {
		*task->wait_context.stat_loc = child->exit_code;
	}

	/* Usually, the task state is set to running by the SIGCHLD, but if the
	 * signal is masked, we still need to return from the wait.
	 */
	task->task_state = TASK_STATE_RUNNING;
}
