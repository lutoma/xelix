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
#include <int/int.h>
#include <log.h>
#include <tasks/scheduler.h>
#include <fs/vfs.h>
#include <printf.h>
#include <panic.h>
#include <errno.h>
#include <variadic.h>
#include <mem/vmem.h>
#include <mem/kmalloc.h>
#include <tty/serial.h>

#include "syscalls.h"

#ifdef SYSCALL_DEBUG
static inline void dbg_print_arg(bool first, uint8_t flags, uint32_t value);
#endif

static inline uintptr_t copy_to_kernel(task_t* task, uint32_t addr, size_t ptr_size) {
	struct vmem_range* vmem_range = vmem_get_range(task->memory_context, addr, false);
	if(!vmem_range) {
		return 0;
	}

	if(ptr_size) {
		uintptr_t ptr_end = addr + ptr_size;
		uintptr_t range_end = vmem_range->virt_start + vmem_range->length;
		//serial_printf("Range %#x - %#x, Pointer %#x - %#x\n", vmem_range->virt_start, range_end, addr, ptr_end);
		if(ptr_end > range_end) {
			serial_printf("oob\n");
			//return 0;
		}
	}

	addr = vmem_translate_ptr(vmem_range, addr, false);
	return addr;
}

static inline void send_strace(task_t* task, isf_t* state, int scnum, uintptr_t* args, uint8_t* flags) {
	vfs_file_t* strace_file = vfs_get_from_id(task->strace_fd, task->strace_observer);
	if(unlikely(!strace_file)) {
		return;
	}

	struct strace {
		uint32_t call;
		uint32_t arg0;
		char arg0_ptrdata[0x50];
		uint32_t arg1;
		char arg1_ptrdata[0x50];
		uint32_t arg2;
		char arg2_ptrdata[0x50];
		uint32_t result;
		uint32_t errno;
	};

	struct strace strace = {
		.call = scnum,
		.result = state->SCREG_RESULT,
		.errno = state->SCREG_ERRNO,
	};

	if(flags[0] & (SCA_STRING | SCA_POINTER)) {
		memcpy(strace.arg0_ptrdata, (void*)args[0], 0x50);
	}
	if(flags[1] & (SCA_STRING | SCA_POINTER)) {
		memcpy(strace.arg1_ptrdata, (void*)args[1], 0x50);
	}
	if(flags[2] & (SCA_STRING | SCA_POINTER)) {
		memcpy(strace.arg2_ptrdata, (void*)args[2], 0x50);
	}

	strace.arg0 = args[0];
	strace.arg1 = args[1];
	strace.arg2 = args[2];
	vfs_write(task->strace_observer, strace_file->num, &strace, sizeof(struct strace));
}

#define call_fail() \
	state->SCREG_RESULT = -1; \
	state->SCREG_ERRNO = EINVAL; \
	return

static void int_handler(isf_t* state) {
	task_t* task = scheduler_get_current();
	if(unlikely(!task)) {
		log(LOG_WARN, "syscall: Got interrupt, but there is no current task.\n");
		call_fail();
	}

	task->task_state = TASK_STATE_SYSCALL;
	uint32_t scnum = state->SCREG_CALLNUM;
	struct syscall_definition def = syscall_table[scnum];

	if(unlikely(scnum >= ARRAY_SIZE(syscall_table) || !def.handler)) {
		call_fail();
	}

	int num_args = 0;
	uint8_t flags[3] = {def.arg0_flags, def.arg1_flags, def.arg2_flags};
	uint32_t args[3] = {state->SCREG_ARG0, state->SCREG_ARG1,
		state->SCREG_ARG2};

	// Translate arguments into kernel memory where needed
	for(int i = 0; i < 3; i++) {
		if(!flags[i]) {
			break;
		}
		num_args++;

		// Pass non-pointer types 1:1
		if(!(flags[i] & (SCA_POINTER | SCA_STRING))) {
			continue;
		}

		/* Get pointer size - From an argument if SCA_SIZE_IN_* is set,
		 * otherwise use the default value
		 */
		size_t ptr_size = 0;
		if(flags[i] & SCA_POINTER) {
			ptr_size = def.ptr_size;
			if(flags[i] & SCA_SIZE_IN_0) {
				ptr_size = MAX(1, ptr_size) * args[0];
			} else if(flags[i] & SCA_SIZE_IN_1) {
				ptr_size = MAX(1, ptr_size) * args[1];
			} else if(flags[i] & SCA_SIZE_IN_2) {
				ptr_size = MAX(1, ptr_size) * args[2];
			}
		}

		if(unlikely((flags[i] & SCA_POINTER) && !ptr_size && !(flags[i] & SCA_NULLOK))) {
			call_fail();
		}

		if(flags[i] & SCA_NULLOK && !args[i]) {
			continue;
		}

		args[i] = copy_to_kernel(task, args[i], ptr_size);
		if(unlikely(!args[i])) {
			call_fail();
		}
	}

#ifdef SYSCALL_DEBUG
	log(LOG_DEBUG, "%d %s: %s(",
		task->pid, task->name,
		def.name);

	dbg_print_arg(true, flags[0], args[0]);
	dbg_print_arg(false, flags[1], args[1]);
	dbg_print_arg(false, flags[2], args[2]);
	serial_printf(")\n");
#endif

	// Shuffle arguments into callback format
	uintptr_t cb_args[5] = {0};
	int aoff = 0;
	if(def.flags & SCF_STATE) {
		cb_args[num_args] = (uintptr_t)state;
		aoff++;
	}

	cb_args[num_args + aoff] = (uintptr_t)task;
	aoff++;

	// Reverse order for cdecl
	for(int i = 0; i < num_args; i++) {
		cb_args[num_args - aoff - i] = args[i];
	}

	task->syscall_errno = 0;
	state->SCREG_RESULT = variadic_call(def.handler, num_args + aoff, cb_args);
	state->SCREG_ERRNO = task->syscall_errno;

	// Only change state back if it hasn't alreay been modified
	if(task->task_state == TASK_STATE_SYSCALL) {
		task->task_state = TASK_STATE_RUNNING;
	}

#ifdef SYSCALL_DEBUG
	log(LOG_DEBUG, "Result: 0x%x, errno: %d\n", state->SCREG_RESULT, state->SCREG_ERRNO);
#endif

	if(unlikely(task->strace_observer && task->strace_fd)) {
		send_strace(task, state, scnum, args, flags);
	}
}

// Check an array to make sure it's NULL-terminated, then copy to kernel space
char** syscall_copy_array(task_t* task, char** array, uint32_t* count) {
	int size = 0;

	for(; size < 200; size++) {
		if(!array[size]) {
			break;
		}
	}

	if(size >= 200) {
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

#ifdef SYSCALL_DEBUG
static inline void dbg_print_arg(bool first, uint8_t flags, uint32_t value) {
	if(flags != 0) {
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
#endif
