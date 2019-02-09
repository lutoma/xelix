/* execve.c: execve syscall
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

#include <tasks/syscall.h>
#include <log.h>
#include <tasks/scheduler.h>
#include <tasks/elf.h>
#include <multiboot.h>
#include <string.h>
#include <memory/kmalloc.h>
#include <memory/vmem.h>
#include <errno.h>

// Check an array to make sure it's NULL-terminated, then copy to kernel space
static char** copy_array(task_t* task, char** array, uint32_t* count) {
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
		new_array[i] = strndup((char*)vmem_translate(task->memory_context, (intptr_t)array[i], false), 200);
	}

	new_array[i] = NULL;

	if(count) {
		*count = size;
	}

	return new_array;
}

SYSCALL_HANDLER(execve)
{
	uint32_t __argc = 0;
	uint32_t __envc = 0;
	char** __argv = copy_array(syscall.task, (char**)syscall.params[1], &__argc);
	char** __env = copy_array(syscall.task, (char**)syscall.params[2], &__envc);

	if(!__argv || !__env) {
		log(LOG_WARN, "execve: array check fail\n");
		return 0;
	}

	//task_reset(syscall.task, syscall.task->parent, (char*)syscall.params[0], __env, __envc, __argv, __argc);
	task_t* new_task = task_new(syscall.task->parent, syscall.task->pid, (char*)syscall.params[0], __env, __envc, __argv, __argc);
	if(elf_load_file(new_task, (void*)syscall.params[0]) == -1) {
		sc_errno = ENOENT;
		return -1;
	}

	memcpy(new_task->files, syscall.task->files, sizeof(new_task->files));
	scheduler_add(new_task);
	syscall.task->task_state = TASK_STATE_REPLACED;
	syscall.task->interrupt_yield = true;
	return 0;
}
