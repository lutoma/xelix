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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

FILE* fopen(const char* path, const char* mode)
{
	uint32_t num;
	asm __volatile__(
		"mov eax, 13;"
		"mov ebx, %1;"
		"mov ecx, %2;"
		"int 0x80;"
		"mov %0, eax;"
	: "=r" (num) : "r" (path), "r" (mode) : "eax", "ebx", "ecx");
	
	if(num == -1)
		return NULL;

	FILE* fd = malloc(sizeof(FILE));
	fd->num = num;
	strcpy(fd->filename, path);
	fd->offset = 0;
	return fd;
}
