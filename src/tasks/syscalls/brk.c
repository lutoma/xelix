/* brk.c: Legacy brk() syscall
 * Copyright © 2011 Fritz Grimpen
 * Copyright © 2015 Lukas Martini
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

#define alignedMemoryPosition() (kmalloc_getMemoryPosition() + 4096 - (kmalloc_getMemoryPosition() % 4096))

SYSCALL_HANDLER(brk)
{
	if (vmem_currentContext == vmem_kernelContext)
	{
		if (syscall.params[0] == 0)
			SYSCALL_RETURN(alignedMemoryPosition());

		size_t allocationSize = syscall.params[0] - alignedMemoryPosition();
		kmalloc_a(allocationSize);

		SYSCALL_RETURN((int)alignedMemoryPosition());
	}

	SYSCALL_FAIL();
}
