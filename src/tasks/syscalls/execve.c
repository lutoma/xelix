/* execve.c: Execve syscall
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

#include "execve.h"
#include <lib/log.h>
#include <tasks/scheduler.h>
#include <tasks/elf.h>

int sys_execve(struct syscall syscall)
{
	// Hardcoded for dash, but doesn't hurt for other processes either
	char* __env[] = { "PS1=[$USER@$HOST $PWD]# ", "HOME=/root", "TERM=dash", "PWD=/", "USER=root", "HOST=default", NULL }; 
	char* __argv[] = { "dash", "-liV", NULL };

	task_t* task = scheduler_getCurrentTask();
	task_t* new_task = elf_load_file((void*)syscall.params[0], __env, __argv, 2);

	//FIXME This is not entirely POSIX compatible as the new process has a different PID.
	if(new_task)
	{
		scheduler_add(new_task);
		scheduler_remove(task);
		return 0;
	}

	return -1;
}
