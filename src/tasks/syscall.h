#pragma once

/* Copyright © 2011-2016 Lukas Martini
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

#include <hw/interrupts.h>
#include <tasks/task.h>
#include <mem/vmem.h>
#include <print.h>

#define SYSCALL_INTERRUPT 0x80
#define SYSCALL_HANDLER(name) uint32_t sys_ ## name (struct syscall syscall)
#define SYSCALL_ARG_RESOLVE 1
#define SYSCALL_ARG_RESOLVE_NULL_OK 2

#define DEFINE_SYSCALL(name) extern uint32_t sys_ ## name (struct syscall syscall);
#define SYS_REDIR(name, fname, args...) \
	static inline uint32_t sys_ ## name (struct syscall syscall) { \
		return fname ( args ); \
	}

#define SYS_DISABLED(name) \
	static inline uint32_t sys_ ## name (struct syscall syscall) {sc_errno = ENOSYS; return -1;}

#define SYSCALL_ENTRY_OG(name, arg0, arg1, arg2) \
	{ \
		.handler = sys_exit, \
		.name = "exit", \
		.arg0 = arg0, \
		.arg1 = arg1, \
		.arg2 = arg2, \
	}

struct syscall {
	int num;
	int params[3];
	isf_t* state;
	task_t* task;
};

typedef uint32_t (*syscall_t)(struct syscall);

struct syscall_definition {
	syscall_t handler;
	char name[50];

	uint8_t arg0;
	uint8_t arg1;
	uint8_t arg2;
};

char** syscall_copy_array(task_t* task, char** array, uint32_t* count);
void syscall_init();
