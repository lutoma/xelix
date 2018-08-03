/* sys_getexecdata.c: Get task data. Called in land crt0.c
 * Copyright © 2011-2018 Lukas Martini
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

#include <tasks/syscall.h>
#include <tasks/scheduler.h>
#include <lib/string.h>

struct execdata {
	uint32_t pid;
	uint32_t ppid;
	uint32_t argc;
	uint32_t envc;

	// Contains argc arguments, then envc environment strings.
	char* argv_environ[];
};

// Return execution data.
SYSCALL_HANDLER(getexecdata)
{
	task_t* task = scheduler_get_current();
	struct execdata* execdata = (struct execdata*)syscall.params[0];

	execdata->pid = task->pid;
	execdata->ppid = task->parent ? task->parent->pid : 0;
	execdata->argc = task->argc;
	execdata->envc = task->envc;

	uint32_t offset = 0;
	for(int i = 0; i < task->argc; i++) {
		strncpy((char*)((intptr_t)execdata->argv_environ + offset), task->argv[i], 200);
		offset += strlen(task->argv[i]) + 1;
	}

	for(int i = 0; i < task->envc; i++) {
		strncpy((char*)((intptr_t)execdata->argv_environ + offset), task->environ[i], 200);
		offset += strlen(task->environ[i]) + 1;
	}

	SYSCALL_RETURN(1);
}
