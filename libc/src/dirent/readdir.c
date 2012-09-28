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

struct dirent* readdir(DIR* dd)
{
	char* name = NULL;
	asm __volatile__(
		"mov eax, 17;"
		"mov ebx, %1;"
		"mov ecx, %2;"
		"int 0x80;"
		"mov %0, eax;"
	: "=r" (name) : "r" (dd->num), "r" (dd->offset) : "eax", "ebx", "ecx");

	if(!name)
		return NULL;

	struct dirent* de = malloc(sizeof(struct dirent));
	if(!de)
		return NULL;

	de->d_ino = 0; // FIXME
	strcpy(de->d_name, name);

	// Increase offset of directory handle
	dd->offset++;
	return de;
}