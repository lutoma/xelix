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

/* Todo: Currently we return a full page for every request, wasting
 * tremendous amounts of memory. This should be fixed.
 */

#include <stddef.h>

void* malloc(size_t size)
{
	void* addr = -1;
	asm("mov eax, 7;"
		"mov ebx, 0;"
		"mov ecx,%0;"
		"mov edx, 0;"
		"int 0x80;	"
		"mov %1,eax;"
	: "=r" (addr) : "r" (size) : "eax", "ebx", "ecx", "edx");
	return addr;
}
