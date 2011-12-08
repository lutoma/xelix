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
#include <string.h>

int fputs(const char* string, FILE* fp)
{
	size_t len = strlen(string);
	asm __volatile__(
		"mov eax, 3;"
		"mov ebx, %0;"
		"mov ecx, %1;"
		"mov edx, %2;" 
		"int 0x80;"
	:: "r" (fp->num), "r" (string), "r" (len) : "eax", "ebx", "ecx", "edx");
	return 0;
}
