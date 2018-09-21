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
#include <string.h>
#include <memory/kmalloc.h>

// Needs to be in this format to stay compatible with newlib
struct newlib_dirent {
	uint32_t d_ino;
	uint32_t d_off;
	uint16_t d_reclen;
    uint8_t d_type;
	char d_name[] __attribute__ ((nonstring));
};

SYSCALL_HANDLER(getdents)
{
	SYSCALL_SAFE_RESOLVE_PARAM(1)

	vfs_file_t* dd = vfs_get_from_id(syscall.params[0]);
	if(!dd) {
		return 0;
	}

	intptr_t buf = (intptr_t)syscall.params[1];
	uint32_t size = syscall.params[2];

	void* kbuf = kmalloc(2048);
	size_t read = vfs_getdents(dd, kbuf, 2048);
	if(!read) {
		kfree(kbuf);
		return 0;
	}

	uint32_t offset = 0;
	vfs_dirent_t* kernel_ent = kbuf;
	while(kernel_ent < kbuf + read) {
		if(!kernel_ent->name_len) {
			goto next;
		}

		uint32_t reclen = sizeof(struct newlib_dirent) + kernel_ent->name_len + 1;
		if(offset + reclen > size) {
			break;
		}

		struct newlib_dirent* ent = (struct newlib_dirent*)(buf + offset);
		ent->d_ino = kernel_ent->inode;
		ent->d_type = kernel_ent->type;
		ent->d_reclen = (uint16_t)reclen;
		memcpy(ent->d_name, kernel_ent->name, kernel_ent->name_len);
		offset += ent->d_reclen;
		ent->d_off = offset;

		char* termname = strndup(kernel_ent->name, kernel_ent->name_len);

		next:
		kernel_ent = (vfs_dirent_t*)((intptr_t)kernel_ent + (intptr_t)kernel_ent->record_len);
	}

	kfree(kbuf);
	return offset;
}
