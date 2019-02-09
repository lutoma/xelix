/* open.c: Open Syscall
 * Copyright © 2011-2015 Lukas Martini
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

#include <fs/vfs.h>
#include <log.h>
#include <string.h>
#include <errno.h>
#include <tasks/syscall.h>

SYSCALL_HANDLER(open)
{
	vfs_file_t* fd = vfs_open((char*)syscall.params[0], syscall.params[1], syscall.task);
	if(!fd) {
		return -1;
	}

	return fd->num;
}
