/* cwd.c: Get/set current working directory
 * Copyright © 2013-2015 Lukas Martini
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

SYSCALL_HANDLER(chdir)
{
	SYSCALL_SAFE_RESOLVE_PARAM(0);

	// FIXME This should seriously check if this directory even exists…
	strncpy(syscall.task->cwd, (char*)syscall.params[0], SCHEDULER_TASK_PATH_MAX);
	SYSCALL_RETURN(0);
}

SYSCALL_HANDLER(getcwd)
{
	SYSCALL_SAFE_RESOLVE_PARAM(0);

	// Maximum return string size
	if(syscall.params[1] > SCHEDULER_TASK_PATH_MAX)
		syscall.params[1] = SCHEDULER_TASK_PATH_MAX;

	strncpy((char*)syscall.params[0], syscall.task->cwd, syscall.params[1]);
	SYSCALL_RETURN(syscall.params[0]);
}
