/* cwd.c: Set current working directory
 * Copyright © 2013-2018 Lukas Martini
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
#include <tasks/task.h>
#include <memory/kmalloc.h>
#include <fs/vfs.h>
#include <string.h>
#include <errno.h>

SYSCALL_HANDLER(chdir)
{
	SYSCALL_SAFE_RESOLVE_PARAM(0);

	vfs_file_t* fp = vfs_open((char*)syscall.params[0], O_RDONLY, syscall.task);
	if(!fp) {
		return -1;
	}

	vfs_stat_t* stat = kmalloc(sizeof(vfs_stat_t));
	if(vfs_stat(fp, stat) != 0) {
		kfree(stat);
		sc_errno = ENOENT;
		return -1;
	}

	if(vfs_mode_to_filetype(stat->st_mode) != FT_IFDIR) {
		sc_errno = ENOTDIR;
		return -1;
	}

	kfree(stat);
	strcpy(syscall.task->cwd, fp->path);
	return 0;
}
