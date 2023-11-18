/* syscall.c: Syscall handling
 * Copyright © 2011-2023 Lukas Martini
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
#include <fs/sysfs.h>
#include <printf.h>
#include <panic.h>
#include <errno.h>
#include <variadic.h>
#include <mem/kmalloc.h>
#include <tty/serial.h>

#include "syscalls.h"

#ifdef CONFIG_SYSCALL_DEBUG
static inline void dbg_print_arg(bool first, uint8_t flags, uint32_t value, uint32_t ovalue);
#endif

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
	state->SCREG_ERRNO = EFAULT; \
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
	vm_alloc_t vmem[3] = {0};
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

		int map_flags = VM_RW | VM_MAP_USER_ONLY;

		/* Get pointer size - From an argument if SCA_SIZE_IN_* is set,
		 * otherwise use the default value
		 */
		int multiplicator = MAX(1, def.ptr_size);
		if(flags[i] & SCA_SIZE_IN_0) {
			ptr_sizes[i] = multiplicator * args[0];
		} else if(flags[i] & SCA_SIZE_IN_1) {
			ptr_sizes[i] = multiplicator * args[1];
		} else if(flags[i] & SCA_SIZE_IN_2) {
			ptr_sizes[i] = multiplicator * args[2];
		} else if(flags[i] & (SCA_STRING | SCA_FLEX_SIZE)) {
			/* If there is no SIZE_IN_* flag, attempt to map up to two pages,
			 * but don't fail if only one could be mapped. This is used for
			 * strings. Later on, code will scan the mapped area to make
			 * sure the string is NULL-terminated.
			 */
			ptr_sizes[i] = PAGE_SIZE * 2;
			map_flags |= VM_MAP_LESS_OK;

		} else {
			ptr_sizes[i] = def.ptr_size;
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

		args[i] = (uint32_t)vm_map(VM_KERNEL, &vmem[i], &task->vmem,
			(void*)args[i], ptr_sizes[i], map_flags);

		if(unlikely(!args[i])) {

			#ifdef CONFIG_SYSCALL_DEBUG
			log(LOG_DEBUG, "%2d %-20s %s(%#x, %#x, %#x)\n", task->pid, task->name,
				def.name, oargs[0], oargs[1], oargs[2]);
			log(LOG_DEBUG, "Result: Call failed - Could not memmap argument %d\n", i);
			#endif

			log(LOG_WARN, "tasks: %d %s: Invalid memory pointer in argument %d to syscall %d %s\n",
				task->pid, task->name, i, scnum, def.name);
			task_signal(task, NULL, SIGSEGV);
			call_fail();
		}

		// Ensure strings are NULL-terminated
		if(flags[i] & SCA_STRING) {
			size_t slen = strnlen((char*)args[i], ptr_sizes[i]);
			if(slen == ptr_sizes[i]) {
				log(LOG_WARN, "tasks: %d %s: Unterminated string in argument %d to syscall %d %s\n",
					task->pid, task->name, i, scnum, def.name);
				task_signal(task, NULL, SIGSEGV);
				call_fail();
			}
		}
	}

#ifdef CONFIG_SYSCALL_DEBUG
	log(LOG_DEBUG, "%2d %-20s %s(", task->pid, task->name, def.name);
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
		if(vmem[i].self) {
			vm_free(&vmem[i]);
		}
	}

	// Only change state back if it hasn't alreay been modified
	if(task->task_state == TASK_STATE_SYSCALL) {
		task->task_state = TASK_STATE_RUNNING;
	}

#ifdef CONFIG_SYSCALL_DEBUG
	log(LOG_DEBUG, "%2d %-20s %s = %d, errno %d\n", task->pid, task->name, def.name,
		state->SCREG_RESULT, state->SCREG_ERRNO);
#endif

	if(unlikely(task->strace_observer && task->strace_fd)) {
		send_strace(task, state, scnum, args, oargs, flags);
	}
}


static inline char* arg_type_name(int flags) {
	if(flags & SCA_INT) {
		return "int";
	} else if(flags & SCA_POINTER) {
		return "void*";
	} else if(flags & SCA_STRING) {
		return "char*";
	}
	return "";
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	size_t rsize = 0;
	for(int i = 0; i < ARRAY_SIZE(syscall_table); i++) {
		struct syscall_definition def = syscall_table[i];
		if(!def.handler) {
			sysfs_printf("%-3d\n", i);
		} else {
			char* hname = addr2name((uintptr_t)def.handler);
			if(!hname) {
				hname = "???";
			}

			sysfs_printf("%-3d %-20s -> %-22s (", i, def.name, hname);
			if(def.arg0_flags) {
				sysfs_printf(arg_type_name(def.arg0_flags));
			}
			if(def.arg1_flags) {
				sysfs_printf(", %s", arg_type_name(def.arg1_flags));
			}
			if(def.arg2_flags) {
				sysfs_printf(", %s", arg_type_name(def.arg2_flags));
			}

			sysfs_printf(")\n");
		}
	}

	return rsize;
}

void syscall_init(void) {
	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("syscalls", &sfs_cb);
	int_register(SYSCALL_INTERRUPT, int_handler, false);
}

#ifdef CONFIG_SYSCALL_DEBUG
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
