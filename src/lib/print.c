/* printf.c: Print and related functions
 * Copyright © 2011 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include "print.h"
#include "generic.h"
#include "string.h"
#include <stdarg.h>

#include <console/interface.h>

// Print a single char
void printChar(const char c)
{
	char s[2];
	s[0] = c;
	s[1] = 0;
	console_write(NULL, s, 1);
}

// Printing a string, formatted with the stuff in the arg array
void vprintf(const char *fmt, void** arg) {
	bool state = false;
	char toa_result[300];

	while (*fmt) {
		if (*fmt == '%') {
			++fmt;
			switch (*fmt) {
				case 'c': printChar(*(char *)arg); break;
				// Print (null) if pointer == NULL.
				case 's': print(*(char **)arg ? *(char **)arg : "(null)"); break;
				case 'b': print(itoa(*(unsigned *)arg, (char*)&toa_result, 2)); break;
				case 'd': print(itoa(*(unsigned *)arg, (char*)&toa_result, 10)); break;
				case 'u': print(utoa(*(unsigned *)arg, (char*)&toa_result, 10)); break;
				case 'y': print(utoa(*(unsigned *)arg, (char*)&toa_result, 16)); break;
				case 'x': print(itoa(*(unsigned *)arg, (char*)&toa_result, 16)); break;
				case 't': print(*(unsigned *)arg ? "true" : "false"); break;
			}

			if(*fmt == '%')
			{
				if(!state)
				{
					default_console->info.current_color.foreground = *(unsigned *)arg;
				} else {
					default_console->info.current_color = default_console->info.default_color;
					--arg;
				}
				state = !state;
			}

			/* p stands for pluralize. If the last parameter wasn't 1,
			 * print an 's'. Doesn't work for all words, but hey.
			 */
			if(*fmt == 'p')
				if(*(unsigned*)(arg-1) != 1)
					printChar('s');

			++arg;
		} else printChar(*fmt);
		++fmt;
	}
}

void printf(const char *fmt, ...) {
	vprintf(fmt, (void**)(&fmt) + 1);
}
