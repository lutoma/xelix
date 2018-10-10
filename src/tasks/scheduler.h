#pragma once

/* Copyright © 2011-2018 Lukas Martini
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
#include <hw/interrupts.h>

enum {
	SCHEDULER_OFF,
	SCHEDULER_INITIALIZING,
	SCHEDULER_INITIALIZED
} scheduler_state;


void scheduler_add(task_t *task);
task_t* scheduler_get_current();
task_t* scheduler_select(isf_t* lastRegs);
void scheduler_init();
void scheduler_remove(task_t *t);

static inline void scheduler_terminate_current() {
	task_t* task = scheduler_get_current();
	if(task) {
		task->task_state = TASK_STATE_TERMINATED;
		task->interrupt_yield = true;
	}
}
