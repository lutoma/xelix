/* execnew.c: Execnew syscall
 * Copyright © 2016-2018 Lukas Martini
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
#include <lib/string.h>
#include <memory/kmalloc.h>

// Check an array to make sure it's NULL-terminated, then copy to kernel space
static int copy_array(char** array) {
	int size = 0;
	for(; size < 200; size++) {
		if(!array[size]) {
			break;
		}
	}

	if(size < 1 || size >= 200) {
		return NULL;
	}

	char** new_array = kmalloc(sizeof(char*) * (size + 1));
	int i = 0;
	for(; i < size; i++) {
		new_array[i] = strndup(array[i], 200);
	}

	new_array[i] = NULL;
	return new_array;
}

SYSCALL_HANDLER(execnew)
{
	SYSCALL_SAFE_RESOLVE_PARAM(0);
	SYSCALL_SAFE_RESOLVE_PARAM(1);
	SYSCALL_SAFE_RESOLVE_PARAM(2);

	char** __argv = copy_array((char**)syscall.params[1]);
	char** __env = copy_array((char**)syscall.params[2]);

	if(!__argv || !__env) {
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
