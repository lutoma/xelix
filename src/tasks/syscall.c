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
static inline void dbg_print_arg(bool first, uint8_t flags, uint32_t value, uint32_t ovalue);
#endif

void sc_ctx_copy(task_t* task, void* kaddr, uintptr_t addr, size_t ptr_size, bool user_to_kernel) {
	uintptr_t off = 0;
	struct vmem_range* cr;

	while(off < ptr_size) {
		cr = vmem_get_range(task->vmem_ctx, addr + off, false);
		uintptr_t paddr = vmem_translate_ptr(cr, addr + off, false);
		size_t copy_size = MIN(ptr_size - off, cr->length - (paddr - cr->phys_start));
		if(!copy_size) {
			break;
		}

		if(user_to_kernel) {
			memcpy((void*)paddr, kaddr + off, copy_size);
		} else {
			memcpy(kaddr + off, (void*)paddr, copy_size);
		}

		off += copy_size;
	}
}

uintptr_t sc_map_to_kernel(task_t* task, uintptr_t addr, size_t ptr_size, bool* copied) {
	struct vmem_range* vmem_range = vmem_get_range(task->vmem_ctx, addr, false);
	if(!vmem_range) {
		return 0;
	}

	uintptr_t ptr_end = addr + ptr_size;
	struct vmem_range* cr = vmem_range;

	for(int i = 1;; i++) {
		uintptr_t virt_end = cr->virt_start + cr->length;
		uintptr_t phys_end = cr->phys_start + cr->length;

		/* We've reached the end of the pointer and the buffer is contiguous in
		 * physical memory, so just pass it directly.
		 */
		if(ptr_end <= virt_end) {
			return vmem_translate_ptr(vmem_range, addr, false);
		}

		// Get next vmem range
		cr = vmem_get_range(task->vmem_ctx, virt_end + PAGE_SIZE * i, false);
		if(!cr) {
			return 0;
		}

		// Check if next range is in adjacent physical pages
		if(cr->phys_start != phys_end) {
			break;
		}
	}

	void* fmb = kmalloc(ptr_size);
	sc_ctx_copy(task, fmb, addr, ptr_size, false);
	*copied = true;
	return (uintptr_t)fmb;
}

static inline void send_strace(task_t* task, isf_t* state, int scnum, uintptr_t* args, uintptr_t* oargs, uint8_t* flags) {
	vfs_file_t* strace_file = vfs_get_from_id(task->strace_fd, task->strace_observer);
	if(unlikely(!strace_file)) {
		return;
	}

	struct strace strace = {
		.call = scnum,
		.result = state->SCREG_RESULT,
		.errno = state->SCREG_ERRNO,
	};

	for(int i = 0; i < 3; i++) {
		strace.args[i] = oargs[i];
		if(flags[i] & (SCA_STRING | SCA_POINTER) && args[i]) {
			memcpy(strace.ptrdata[i], (void*)args[i], 0x50);
		}
	}
	vfs_write(task->strace_observer, strace_file->num, &strace, sizeof(struct strace));
}

#define call_fail() \
	state->SCREG_RESULT = -1; \
	state->SCREG_ERRNO = EINVAL; \
	return

static void int_handler(task_t* task, isf_t* state, int num) {
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
	bool copied[3] = {0};
	size_t ptr_sizes[3] = {0};
	uint8_t flags[3] = {def.arg0_flags, def.arg1_flags, def.arg2_flags};
	uint32_t args[3] = {state->SCREG_ARG0, state->SCREG_ARG1,
		state->SCREG_ARG2};
	uint32_t oargs[3] = {state->SCREG_ARG0, state->SCREG_ARG1,
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
		if(flags[i] & SCA_STRING) {
			// FIXME
			ptr_sizes[i] = 0x1;
		} else {
			ptr_sizes[i] = def.ptr_size;
			if(flags[i] & SCA_SIZE_IN_0) {
				ptr_sizes[i] = MAX(1, ptr_sizes[i]) * args[0];
			} else if(flags[i] & SCA_SIZE_IN_1) {
				ptr_sizes[i] = MAX(1, ptr_sizes[i]) * args[1];
			} else if(flags[i] & SCA_SIZE_IN_2) {
				ptr_sizes[i] = MAX(1, ptr_sizes[i]) * args[2];
			}
		}

		if(unlikely((flags[i] & SCA_POINTER) && !ptr_sizes[i] && !(flags[i] & SCA_NULLOK))) {
			call_fail();
		}

		if(flags[i] & SCA_NULLOK && !args[i]) {
			continue;
		}

		if(!ptr_sizes[i]) {
			call_fail();
		}

		args[i] = sc_map_to_kernel(task, args[i], ptr_sizes[i], &copied[i]);
		if(unlikely(!args[i])) {
			call_fail();
		}
	}

#ifdef SYSCALL_DEBUG
	log(LOG_DEBUG, "%d %s: %s(",
		task->pid, task->name,
		def.name);

	dbg_print_arg(true, flags[0], args[0], oargs[0]);
	dbg_print_arg(false, flags[1], args[1], oargs[1]);
	dbg_print_arg(false, flags[2], args[2], oargs[2]);
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

	for(int i = 0; i < 3; i++) {
		if(!copied[i]) {
			continue;
		}

		sc_ctx_copy(task, (void*)args[i], oargs[i], ptr_sizes[i], true);
		kfree((void*)args[i]);
	}

	// Only change state back if it hasn't alreay been modified
	if(task->task_state == TASK_STATE_SYSCALL) {
		task->task_state = TASK_STATE_RUNNING;
	}

#ifdef SYSCALL_DEBUG
	log(LOG_DEBUG, "Result: 0x%x, errno: %d\n", state->SCREG_RESULT, state->SCREG_ERRNO);
#endif

	if(unlikely(task->strace_observer && task->strace_fd)) {
		send_strace(task, state, scnum, args, oargs, flags);
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
		new_array[i] = strndup((char*)vmem_translate(task->vmem_ctx, (intptr_t)array[i], false), 200);
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
static inline void dbg_print_arg(bool first, uint8_t flags, uint32_t value, uint32_t ovalue) {
	if(flags != 0) {
		char* fmt = "%d";
		if(flags & SCA_POINTER) {
			fmt = "%#x";
		} else if(flags & SCA_STRING) {
			fmt = "\"%s\"";
		}

		if(!first) {
			serial_printf(", ");
		}
		serial_printf(fmt, flags & SCA_STRING ? value : ovalue);
	}
}
#endif
