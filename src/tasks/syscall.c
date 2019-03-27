/* syscall.c: Syscall handling
 * Copyright © 2011-2019 Lukas Martini
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

#include "syscall.h"
#include <hw/interrupts.h>
#include <log.h>
#include <tasks/scheduler.h>
#include <print.h>
#include <panic.h>
#include <errno.h>
#include <mem/vmem.h>
#include <mem/kmalloc.h>

#include "syscalls.h"

static inline int handle_arg_flags(task_t* task, uint32_t* arg, uint8_t flags) {
	if(flags & SCA_TRANSLATE) {
		if(flags & SCA_NULLOK && !*arg) {
			return 0;
		}

		*arg = vmem_translate(task->memory_context, *arg, false);
		if(!*arg) {
			return -1;
		}
	}

	return 0;
}

static inline void print_arg(bool first, uint8_t flags, uint32_t value) {
	if(flags != SCA_UNUSED) {
		char* fmt = "%d";
		if(flags & SCA_POINTER) {
			fmt = "%#x";
		}
		if(flags & SCA_STRING) {
			fmt = "\"%s\"";
		}

		if(!first) {
			serial_printf(", ");
		}
		serial_printf(fmt, value);
	}

}

static void int_handler(isf_t* state) {
	task_t* task = scheduler_get_current();
	if(!task) {
		log(LOG_WARN, "syscall: Got interrupt, but there is no current task.\n");
		return;
	}

	task->task_state = TASK_STATE_SYSCALL;
	uint32_t scnum = state->eax;
	uint32_t arg0 = state->ebx;
	uint32_t arg1 = state->ecx;
	uint32_t arg2 = state->edx;

	struct syscall_definition def = syscall_table[scnum];
	if (scnum >= sizeof(syscall_table) / sizeof(struct syscall_definition) || !def.handler) {
		state->eax = -1;
		state->ebx = EINVAL;
		return;
	}

	if(handle_arg_flags(task, &arg0, def.arg0_flags) == -1 ||
		handle_arg_flags(task, &arg1, def.arg1_flags) == -1 ||
		handle_arg_flags(task, &arg2, def.arg2_flags) == -1) {

		state->eax = -1;
		state->ebx = EINVAL;
		return;
	}

#ifdef SYSCALL_DEBUG
	task_t* cur = scheduler_get_current();
	log(LOG_DEBUG, "%d %s: %s(",
		cur->pid, cur->name,
		def.name);

	print_arg(true, def.arg0_flags, arg0);
	print_arg(false, def.arg1_flags, arg1);
	print_arg(false, def.arg2_flags, arg2);
	serial_printf(")\n");
#endif

	task->syscall_errno = 0;
	if(def.arg2_flags != SCA_UNUSED) {
		if(def.flags & SCF_TASKEND) {
			state->eax = def.handler(arg0, arg1, arg2, task);
		} else if(def.flags & SCF_STATE) {
			state->eax = def.handler((uint32_t)task, (uint32_t)state, arg0, arg1, arg2);
		} else {
			state->eax = def.handler((uint32_t)task, arg0, arg1, arg2);
		}
	} else if(def.arg1_flags != SCA_UNUSED) {
		if(def.flags & SCF_TASKEND) {
			state->eax = def.handler(arg0, arg1, task);
		} else if(def.flags & SCF_STATE) {
			state->eax = def.handler((uint32_t)task, (uint32_t)state, arg0, arg1);
		} else {
			state->eax = def.handler((uint32_t)task, arg0, arg1);
		}
	} else if(def.arg0_flags != SCA_UNUSED) {
		if(def.flags & SCF_TASKEND) {
			state->eax = def.handler(arg0, task);
		} else if(def.flags & SCF_STATE) {
			state->eax = def.handler((uint32_t)task, (uint32_t)state, arg0);
		} else {
			state->eax = def.handler((uint32_t)task, arg0);
		}
	} else {
		if(def.flags & SCF_STATE) {
			state->eax = def.handler((uint32_t)task, (uint32_t)state);
		} else {
			state->eax = def.handler((uint32_t)task);
		}
	}
	state->ebx = task->syscall_errno;

	// Only change state back if it hasn't alreay been modified
	if(task->task_state == TASK_STATE_SYSCALL) {
		task->task_state = TASK_STATE_RUNNING;
	}

#ifdef SYSCALL_DEBUG
	log(LOG_DEBUG, "Result: 0x%x, errno: %d\n", state->eax, state->ebx);
#endif
}

// Check an array to make sure it's NULL-terminated, then copy to kernel space
char** syscall_copy_array(task_t* task, char** array, uint32_t* count) {
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

void syscall_init() {
	interrupts_register(SYSCALL_INTERRUPT, int_handler, true);
}
