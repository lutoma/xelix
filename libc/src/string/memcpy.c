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

// Will some day™ be replaced by a faster one. Maybe.

#include <stdint.h>

void memcpy(void* dest, void* src, uint32_t size)
{
	uint8_t* from = (uint8_t*) src;
	uint8_t* to = (uint8_t*) dest;

	while(size > 0)
	{
		*to = *from;
		
		size--;
		from++;
		to++;
	}
}
