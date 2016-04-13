/* open.c: Opendir Syscall
 * Copyright © 2012-2015 Lukas Martini
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
#include <lib/log.h>
#include <tasks/syscall.h>

SYSCALL_HANDLER(opendir)
{
	SYSCALL_SAFE_RESOLVE_PARAM(0);
	SYSCALL_RETURN(vfs_dir_open((char*)syscall.params[0])->num);
}
	