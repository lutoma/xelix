/* sys_getexecdata.c: Get argv, argc, environ
 * Copyright © 2011-2016 Lukas Martini
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

// Return execution data.
SYSCALL_HANDLER(getexecdata)
{
	task_t* proc = scheduler_get_current();
	char** argv = proc->argv;
	char** environ = proc->environ;

	SYSCALL_SAFE_REVERSE_RESOLVE(argv);
	SYSCALL_SAFE_REVERSE_RESOLVE(environ);

	switch(syscall.params[0])
	{
		case 0: SYSCALL_RETURN(proc->argc);;
		case 1: SYSCALL_RETURN((int)argv);;
		case 2: SYSCALL_RETURN((int)environ);;
	}

	SYSCALL_RETURN(0);
}
