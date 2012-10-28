/* Copyright © 2011 Lukas Martini
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

DIR* opendir(const char* path)
{
	uint32_t num;
	asm __volatile__(
		"mov eax, 16;"
		"mov ebx, %1;"
		"int 0x80;"
		"mov %0, eax;"
	: "=r" (num) : "r" (path) : "eax", "ebx");
	
	if(num == -1)
		return NULL;

	DIR* dd = malloc(sizeof(DIR));
	dd->num = num;
	dd->offset = 0;
	strcpy(dd->path, path);
	return dd;
}