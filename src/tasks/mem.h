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

#include <tasks/task.h>

#define TASK_STACK_LOCATION 0xc0000000
#define TASK_MEM_FORK	0x1
#define TASK_MEM_FREE	0x2

enum task_mem_section {
	TMEM_SECTION_NONE,
	TMEM_SECTION_STACK,   /* Initial stack */
	TMEM_SECTION_CODE,    /* Contains program code and is read-only */
	TMEM_SECTION_DATA,    /* Contains static data */
	TMEM_SECTION_HEAP,    /* Allocated by brk(2) at runtime */
	TMEM_SECTION_KERNEL,  /* Contains kernel-internal data */
} section;

struct task_mem {
	struct task_mem* next;
	void* virt_addr;
	void* phys_addr;
	uint32_t len;
	enum task_mem_section section;
	int flags;
};

int task_page_fault_cb(task_t* task, uintptr_t addr);
void task_memcpy(task_t* task, void* kaddr, void* addr, size_t ptr_size, bool user_to_kernel);
void* task_memmap(task_t* task, void* addr, size_t ptr_size, bool* copied);
char** task_copy_strings(task_t* task, char** array, uint32_t* count);
void* task_sbrk(task_t* task, int32_t length, int32_t l2);

#define task_add_mem_flat(task, start, size, section, flags) \
	task_add_mem(task, start, start, size, section, flags)
void task_add_mem(task_t* task, void* virt_start, void* phys_start,
	uint32_t size, enum task_mem_section section, int flags);


static inline char* task_mem_section_verbose(enum task_mem_section section) {
	char* names[] = {
		"None",
		"Stack",   /* Initial stack */
		"Code",    /* Contains program code and is read-only */
		"Data",    /* Contains static data */
		"Heap",    /* Allocated by brk(2) at runtime */
		"Kernel",  /* Contains kernel-internal data */
	};

	if(section < ARRAY_SIZE(names)) {
		return names[section];
	}
	return NULL;
}
