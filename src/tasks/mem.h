#pragma once

/* Copyright © 2011-2020 Lukas Martini
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

#include <tasks/task.h>

// FIXME map below binary
#define TASK_STACK_LOCATION 0xc0000000

int task_page_fault_cb(task_t* task, void* addr);
char** task_copy_strings(task_t* task, char** array, uint32_t* count);
void* task_sbrk(task_t* task, int32_t length);
void task_free(task_t* task);
