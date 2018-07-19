/* read.c: Readdir Syscall
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

#include <tasks/syscall.h>
#include <fs/vfs.h>
#include <lib/log.h>
#include <lib/print.h>

SYSCALL_HANDLER(readdir)
{
	vfs_dir_t* dd = vfs_get_dir_from_id(syscall.params[0]);
	if(dd == NULL) {
		SYSCALL_FAIL();
	}

	SYSCALL_RETURN((int)vfs_dir_read(dd, syscall.params[1]));
}
