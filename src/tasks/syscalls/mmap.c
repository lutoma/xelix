/* mmap.c: Implementation of the POSIX mmap syscall
 * Copyright © 2013-2015 Lukas Martini
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
#include <memory/vmem.h>
#include <memory/kmalloc.h>
#include <tasks/scheduler.h>
#include <lib/log.h>

SYSCALL_HANDLER(mmap)
{
	size_t length = VMEM_ALIGN(syscall.params[1]);

	void* addr = kmalloc_a(length);
	task_t* task = scheduler_get_current();

	for(intptr_t i = 0; i < length; i += PAGE_SIZE) {
		struct vmem_page* page = vmem_new_page();
		page->section = VMEM_SECTION_MMAP;
		page->cow = 0;
		page->allocated = 1;
		page->virt_addr = (intptr_t)addr + i;
		page->phys_addr = (intptr_t)addr + i;
		vmem_add_page(task->memory_context, page);
	}

	SYSCALL_RETURN((intptr_t)addr);
}
