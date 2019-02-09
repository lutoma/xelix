/* read.c: Read Syscall
 * Copyright © 2011-2016 Lukas Martini
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

#include <console/console.h>
#include <fs/vfs.h>
#include <errno.h>
#include <tasks/syscall.h>

SYSCALL_HANDLER(read)
{
	vfs_file_t* fd = vfs_get_from_id(syscall.params[0], syscall.task);
	if(!fd) {
		sc_errno = EBADF;
		return -1;
	}

	return vfs_read((void*)syscall.params[1], syscall.params[2], fd);
}
