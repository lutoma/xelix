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

void vfprintf(FILE* fp, const char *fmt, void** arg)
{
	while (*fmt) {
		if (*fmt == '%') {
			++fmt;
			switch (*fmt) {
				case 'c': putc(*(char *)arg, fp); break;
				// Print (null) if pointer == NULL.
				case 's': fputs(*(char**)arg ? *(char**)arg : "(null)", fp); break;
				case 'b': fputs(itoa(*(unsigned*)arg, malloc(intlen(*(unsigned*)arg) * sizeof(char)), 2), fp); break;
				case 'd': fputs(itoa(*(unsigned*)arg, malloc(intlen(*(unsigned*)arg) * sizeof(char)), 10), fp); break;
				case 'x': fputs(itoa(*(unsigned*)arg, malloc(intlen(*(unsigned*)arg) * sizeof(char)), 16), fp); break;
				case 't': fputs(*(unsigned*)arg ? "true" : "false", fp); break;
			}

			++arg;
		} else putc(*fmt, fp);
		++fmt;
	}
}
