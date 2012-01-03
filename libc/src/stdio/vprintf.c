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
				case 's': fputs(*(char**)arg ? *(char**)arg : "(null)", stdout); break;
				case 'b': fputs(itoa(*(unsigned*)arg, malloc(intlen(*(unsigned*)arg) * sizeof(char)), 2), stdout); break;
				case 'd': fputs(itoa(*(unsigned*)arg, malloc(intlen(*(unsigned*)arg) * sizeof(char)), 10), stdout); break;
				case 'x': fputs(itoa(*(unsigned*)arg, malloc(intlen(*(unsigned*)arg) * sizeof(char)), 16), stdout); break;
				case 't': fputs(*(unsigned*)arg ? "true" : "false", stdout); break;
			}

			++arg;
		} else putchar(*fmt);
		++fmt;
	}
}
