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

#include <hw/interrupts.h>
#include <tasks/task.h>
#include <mem/vmem.h>
#include <print.h>

#define SYSCALL_INTERRUPT 0x80

#define SCF_TASKEND 1
#define SCF_STATE 2

#define SCA_UNUSED 0
#define SCA_TRANSLATE 1
#define SCA_NULLOK 2
#define SCA_INT 4
#define SCA_POINTER 8
#define SCA_STRING 16

typedef uint32_t (*syscall_cb)(uint32_t, ...);
struct syscall_definition {
	char name[50];
	syscall_cb handler;
	uint8_t flags;

	uint8_t arg0_flags;
	uint8_t arg1_flags;
	uint8_t arg2_flags;
};

char** syscall_copy_array(task_t* task, char** array, uint32_t* count);
void syscall_init();
