/* getdents.c: Get directory entries
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

// This syscall is used by the newlib posix handlers for readdir, opendir, etc.

#include <tasks/syscall.h>
#include <fs/vfs.h>
#include <lib/log.h>
#include <lib/print.h>
#include <memory/kmalloc.h>

// Needs to be in this format to stay compatible with newlib
struct newlib_dirent {
	uint32_t d_ino;
	uint32_t d_off;
	uint16_t d_reclen;
    uint8_t d_type;
	char d_name[];
};

SYSCALL_HANDLER(getdents)
{
	SYSCALL_SAFE_RESOLVE_PARAM(1)

	vfs_file_t* dd = vfs_get_from_id(syscall.params[0]);
	if(!dd) {
		SYSCALL_RETURN(0);
	}

	intptr_t buf = (intptr_t)syscall.params[1];
	uint32_t size = syscall.params[2];

	vfs_dirent_t* kbuf = kmalloc(1024);
	if(!vfs_getdents(dd, kbuf, 1024)) {
		kfree(kbuf);
		SYSCALL_RETURN(0);
	}

	uint32_t offset = 0;
	vfs_dirent_t* kernel_ent = kbuf;
	while(kernel_ent->name_len) {
		uint32_t reclen = sizeof(struct newlib_dirent) + kernel_ent->name_len + 1;

		if(offset + reclen > size) {
			break;
		}

		struct newlib_dirent* ent = (struct newlib_dirent*)(buf + offset);
		ent->d_ino = kernel_ent->inode;
		ent->d_type = kernel_ent->type;
		ent->d_reclen = (uint16_t)reclen;
		strncpy(ent->d_name, kernel_ent->name, kernel_ent->name_len);
		offset += ent->d_reclen;
		ent->d_off = offset;

		kernel_ent = (vfs_dirent_t*)((intptr_t)kernel_ent + kernel_ent->record_len);
	}

	kfree(kbuf);
	SYSCALL_RETURN(offset);
}
