/* Copyright © 2012 Lukas Martini
 *
 * This file is part of Xlibc.
 *
 * Xlibc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Xlibc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Xlibc. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stddef.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static struct dirent* _create_dirent(char* name, ino_t ino)
{
	struct dirent* de = malloc(sizeof(struct dirent));
	if(!de)
		return NULL;

	de->d_ino = ino;
	strcpy(de->d_name, name);
	return de;
}

struct dirent* readdir(DIR* dd)
{
	// Add . and .. (The Xelix syscall doesn't return those, but POSIX fnord).
	if(dd->offset < 2)
	{
		dd->offset++;

		// 1 since we just did a offset++
		if(dd->offset == 1)
			return _create_dirent(".", 0);

		return _create_dirent("..", 0);
	}

	char* name = NULL;
	asm __volatile__(
		"mov eax, 17;"
		"mov ebx, %1;"
		"mov ecx, %2;"
		"int 0x80;"
		"mov %0, eax;"
	: "=r" (name) : "r" (dd->num), "r" (dd->offset - 2) : "eax", "ebx", "ecx");

	if(!name)
		return NULL;

	dd->offset++;

	// FIXME set correct inode
	return _create_dirent(name, 0);
}