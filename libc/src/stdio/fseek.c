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

#include <stdio.h>

int fseek(FILE* fp, long int offset, int origin)
{
	int ret = -1;
	asm __volatile__(
		"mov eax, 15;"
		"mov ebx, %1;"
		"mov ecx, %2;"
		"mov edx, %3;" 
		"int 0x80;"
		"mov %0, eax;"
	: "=r" (ret) : "r" (fp->num), "r" (offset), "r" (origin) : "eax", "ebx", "ecx", "edx");
	return ret;
}