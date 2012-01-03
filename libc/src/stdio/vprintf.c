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

// Should be replaced

#include <stdio.h>
#include <stdlib.h>

void vprintf(const char *fmt, void** arg)
{
	while (*fmt) {
		if (*fmt == '%') {
			++fmt;
			switch (*fmt) {
				case 'c': putchar(*(char *)arg); break;
				// Print (null) if pointer == NULL.
				case 's': print(*(char**)arg ? *(char**)arg : "(null)"); break;
				case 'b': print(itoa(*(unsigned*)arg, malloc(intlen(*(unsigned*)arg) * sizeof(char)), 2)); break;
				case 'd': print(itoa(*(unsigned*)arg, malloc(intlen(*(unsigned*)arg) * sizeof(char)), 10)); break;
				case 'x': print(itoa(*(unsigned*)arg, malloc(intlen(*(unsigned*)arg) * sizeof(char)), 16)); break;
				case 't': print(*(unsigned*)arg ? "true" : "false"); break;
			}

			++arg;
		} else putchar(*fmt);
		++fmt;
	}
}
