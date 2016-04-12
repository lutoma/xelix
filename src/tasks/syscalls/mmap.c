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

SYSCALL_HANDLER(mmap)
{
	//void *addr = (void *)syscall.params[0];
	size_t length = syscall.params[1];
	/* Ignored:
	int readonly = syscall.params[2];
	int flags =	syscall.params[3];
	int fd = syscall->params[4];
	int offset = syscall->params[5];
	*/

	// Hack until the paging stuff works.
	void* addr = kmalloc_a(length);

	task_t* task = scheduler_get_current();

	struct vmem_page* page = vmem_new_page();
	page->section = VMEM_SECTION_MMAP;
	page->cow = 0;
	page->allocated = 1;
	page->virt_addr = addr;
	page->phys_addr = addr;
	vmem_add_page(task->memory_context, page);

	SYSCALL_RETURN((int)addr);
}
