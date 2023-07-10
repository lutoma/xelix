#pragma once

/* Copyright © 2011-2019 Lukas Martini
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

#include <int/int.h>
#include <tasks/task.h>

#define SYSCALL_INTERRUPT 0x80

#define SCF_STATE 1

#define SCA_INT 1
#define SCA_POINTER 2
#define SCA_STRING 4
#define SCA_NULLOK 8
#define SCA_SIZE_IN_0 16
#define SCA_SIZE_IN_1 32
#define SCA_SIZE_IN_2 64
#define SCA_FLEX_SIZE 128

#ifdef __i386__
	#define SCREG_CALLNUM eax
	#define SCREG_RESULT eax
	#define SCREG_ERRNO ebx
	#define SCREG_ARG0 ebx
	#define SCREG_ARG1 ecx
	#define SCREG_ARG2 edx
#endif


struct strace {
	uint32_t call;
	uint32_t result;
	uint32_t errno;
	uintptr_t args[3];
	char ptrdata[3][0x50];
};

typedef uint32_t (*syscall_cb)(uint32_t, ...);
struct syscall_definition {
	char name[50];
	syscall_cb handler;
	uint8_t flags;

	uint8_t arg0_flags;
	uint8_t arg1_flags;
	uint8_t arg2_flags;
	size_t ptr_size;
};

char** syscall_copy_array(task_t* task, char** array, uint32_t* count);
void syscall_init();
