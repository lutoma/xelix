/* brk.c: Legacy brk() syscall
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

#include "brk.h"
#include <memory/vm.h>
#include <memory/kmalloc.h>

#define alignedMemoryPosition() (kmalloc_getMemoryPosition() + 4096 - (kmalloc_getMemoryPosition() % 4096))

int sys_brk(struct syscall syscall)
{
	if (vm_currentContext == vm_kernelContext)
	{
		if (syscall->params[0] == 0)
			return alignedMemoryPosition();

		size_t allocationSize = syscall->params[0] - alignedMemoryPosition();
		kmalloc_a(allocationSize);

		return (int)alignedMemoryPosition();
	}

	return -1;
}
