/* sys_getexecdata.c: Get argv, argc, environ
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

#include "getexecdata.h"
#include <tasks/syscall.h>
#include <tasks/scheduler.h>

// Return execution data.
int sys_getexecdata(struct syscall syscall)
{
	task_t* proc = scheduler_get_current();

	switch(syscall.params[0])
	{
		case 0: return proc->argc;;
		case 1: return (int)proc->argv;;
		case 2: return (int)proc->environ;;
	}

	return NULL;
}

