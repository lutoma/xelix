/* stat.c: Stat files
 * Copyright © 2018 Lukas Martini
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

SYSCALL_HANDLER(stat)
{
	SYSCALL_SAFE_RESOLVE_PARAM(1)
	vfs_file_t* fd = vfs_get_from_id(syscall.params[0]);
	if(!fd) {
		SYSCALL_RETURN(0);
	}

	SYSCALL_RETURN(vfs_stat(fd, (vfs_stat_t*)syscall.params[1]));
}
