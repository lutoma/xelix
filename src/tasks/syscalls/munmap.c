/* munmap.c: Implementation of the munmap syscall
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

#include <tasks/syscall.h>
#include <memory/vmem.h>
#include <memory/paging.h>

int sys_munmap(struct syscall syscall)
{
	void *addr = (void*)syscall.params[0];
	size_t length = syscall.params[1];

	int pages = length / 4096;
	for (int i = 0; i < pages; ++i)
		vmem_rm_page_virt(vmem_currentContext, (void*)addr + i * 4096);

	return 0;
}
