/* write.c: Write Syscall
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

#include <console/interface.h>
#include <memory/vmem.h>
#include <tasks/syscall.h>

int sys_write(struct syscall syscall)
{
	syscall.params[1] = (int)task_resolve_address(syscall.params[1]);
	if(!syscall.params[1])
		return -1;

	if (syscall.params[0] == 1 || syscall.params[0] == 2)
		return console_write(NULL, (char*)syscall.params[1], syscall.params[2]);

	return -1;
}

