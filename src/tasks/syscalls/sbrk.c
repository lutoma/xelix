/* sbrk.c: Implementation of the POSIX sbrk syscall
 * Copyright © 2013-2018 Lukas Martini
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
#include <log.h>
#include <string.h>

SYSCALL_HANDLER(sbrk)
{
	size_t length = VMEM_ALIGN(syscall.params[1]);
	task_t* task = syscall.task;

	if(length < 0 || length > 0x500000) {
		return -1;
	}

	if(!length) {
		return (intptr_t)task->sbrk;
	}

	void* phys_addr = tmalloc_a(length, task);
	if(!phys_addr) {
		return -1;
	}

	bzero(phys_addr, length);

	void* virt_addr = task->sbrk;
	task->sbrk += length;

	for(intptr_t i = 0; i < length; i += PAGE_SIZE) {
		struct vmem_page* opage = vmem_get_page_virt(task->memory_context, (void*)((intptr_t)virt_addr + i));
		if(opage) {
			vmem_rm_page_virt(task->memory_context, (void*)((intptr_t)virt_addr + i));
		}

		struct vmem_page* page = vmem_new_page();
		page->section = VMEM_SECTION_HEAP;
		page->cow = 0;
		page->allocated = 1;
		page->virt_addr = (void*)((intptr_t)virt_addr + i);
		page->phys_addr = (void*)((intptr_t)phys_addr + i);
		vmem_add_page(task->memory_context, page);
	}

	return (intptr_t)virt_addr;
}
