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
#include <lib/log.h>

SYSCALL_HANDLER(sbrk)
{
	size_t length = VMEM_ALIGN(syscall.params[1]);
	task_t* task = scheduler_get_current();

	if(length < 0) {
		SYSCALL_RETURN(-1);
	}

	if(!length) {
		SYSCALL_RETURN(task->sbrk);
	}

	void* phys_addr = tmalloc_a(length, task);
	if(!phys_addr) {
		SYSCALL_RETURN(-1);
	}

	bzero(phys_addr, length);

	void* virt_addr = task->sbrk;
	task->sbrk += length;

	task_memory_allocation_t* ta = kmalloc(sizeof(task_memory_allocation_t));
	if(!ta) {
		kfree(phys_addr);
		SYSCALL_RETURN(-1);
	}

	ta->phys_addr = phys_addr;
	ta->virt_addr = virt_addr;
	ta->next = task->memory_allocations;
	task->memory_allocations = ta;

	for(intptr_t i = 0; i < length; i += PAGE_SIZE) {
		struct vmem_page* opage = vmem_get_page_virt(task->memory_context, (void*)((intptr_t)virt_addr + i));
		if(opage) {
			vmem_rm_page_virt(task->memory_context, (void*)((intptr_t)virt_addr + i));
		}

		struct vmem_page* page = vmem_new_page();
		page->section = VMEM_SECTION_MMAP;
		page->cow = 0;
		page->allocated = 1;
		page->virt_addr = (void*)((intptr_t)virt_addr + i);
		page->phys_addr = (void*)((intptr_t)phys_addr + i);
		vmem_add_page(task->memory_context, page);
	}

	serial_printf("sbrk phys 0x%x to virt 0x%x\n", phys_addr, virt_addr);
	SYSCALL_RETURN((intptr_t)virt_addr);
}
