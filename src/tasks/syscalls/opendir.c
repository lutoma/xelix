/* open.c: Opendir Syscall
 * Copyright © 2012 Lukas Martini
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

#include "write.h"
#include <fs/vfs.h>
#include <lib/log.h>
#include <tasks/syscall.h>

int sys_opendir(struct syscall syscall)
{
	syscall.params[0] = (int)task_resolve_address(syscall.params[0]);
	if(!syscall.params[0])
		return -1;

	return vfs_dir_open((char*)syscall.params[0])->num;
}
	