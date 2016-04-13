/* execve.c: Execnew syscall
 * Copyright © 2016 Lukas Martini
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
#include <lib/log.h>
#include <tasks/scheduler.h>
#include <tasks/elf.h>
#include <lib/multiboot.h>

// Check an array to make sure it's NULL-terminated.
static bool check_array(char** array) {
	for(int i = 0; i < 200; i++) {
		if(array[i] == NULL)
			return true;
	}
	return false;
}

SYSCALL_HANDLER(execnew)
{
	SYSCALL_SAFE_RESOLVE_PARAM(0);

	char** __argv = (char**)syscall.params[1];
	char** __env = (char**)syscall.params[2];

	if(!check_array(__argv) || !check_array(__env)) {
		log(LOG_WARN, "execnew: array check fail\n");
		SYSCALL_RETURN(0);
	}

	task_t* new_task = elf_load_file((void*)syscall.params[0], __env, __argv, 2);

	if(!new_task) {
		SYSCALL_RETURN(0);
	}

	new_task->parent = scheduler_get_current();
	scheduler_add(new_task);

	SYSCALL_RETURN(new_task->pid);
}
