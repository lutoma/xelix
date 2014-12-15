/* cwd.c: Get/set current working directory
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

#include <tasks/syscall.h>
#include <tasks/scheduler.h>
#include <lib/string.h>

int sys_chdir(struct syscall syscall)
{
	syscall.params[0] = (int)task_resolve_address(syscall.params[0]);
	if(!syscall.params[0])
		return -1;

	// FIXME This should seriously check if this directory even exists…

	task_t* current = scheduler_get_current();
	strncpy(current->cwd, (char*)syscall.params[0], SCHEDULER_TASK_PATH_MAX);
	return 0;
}

int sys_getcwd(struct syscall syscall)
{
	syscall.params[0] = (int)task_resolve_address(syscall.params[0]);
	if(!syscall.params[0])
		return -1;

	// Maximum return string size
	if(syscall.params[1] > SCHEDULER_TASK_PATH_MAX)
		syscall.params[1] = SCHEDULER_TASK_PATH_MAX;

	task_t* current = scheduler_get_current();
	strncpy((char*)syscall.params[0], current->cwd, syscall.params[1]);
	return syscall.params[0];
}
