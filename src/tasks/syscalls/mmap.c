/* mmap.c: Implementation of the POSIX-compilant mmap syscall
 * Copyright © 2011 Fritz Grimpen
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

#include <memory/vm.h>
#include <memory/paging.h>
#include <memory/kmalloc.h>
#include "mmap.h"

int sys_mmap(struct syscall syscall)
{
	void *addr = (void *)syscall.params[0];
	size_t length = syscall.params[1];
	int readonly = syscall.params[2];
	/* Ignored:
	int flags =	syscall.params[3];
	int fd = syscall->params[4];
	int offset = syscall->params[5];
	*/

	// Hack until the paging stuff works.
	return (int)kmalloc(length);

	if (addr == NULL)
	{
		uint32_t counter = 4096;
		uint32_t tmpLength = length;
		while (1)
		{
			struct vm_page *currPage = vm_get_page_virt(vm_currentContext, (void *)counter);

			if (currPage == NULL && length == 0)
			{
				addr = (void *)currPage;
				break;
			}
			else if (currPage == NULL)
				tmpLength -= 4096;
			else
				tmpLength = length;

			counter += 4096;
		}
	}

	int newPages = length / 4096;
	for (int i = 0; i < newPages; ++i)
	{
		struct vm_page *newPage = vm_new_page();
		newPage->section = VM_SECTION_MMAP;
		newPage->readonly = readonly;
		newPage->allocated = 0;
		newPage->virt_addr = addr + i * 4096;

		vm_add_page(vm_currentContext, newPage);
	}

	return (int) addr;
}
