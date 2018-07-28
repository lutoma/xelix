/* printf.c: Print and related functions
 * Copyright © 2011-2018 Lukas Martini
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

void putchar(const char c) {
	char s[2];
	s[0] = c;
	s[1] = 0;
	console_write(NULL, s, 1);
}

void print(const char* s) {
	console_write(NULL, s, strlen(s));
}

void _vprintf(const char *fmt, void** arg, void (print_cb)(const char*), void (putchar_cb)(const char)) {
	bool state = false;
	char toa_result[300];

	while(*fmt) {
		if(unlikely(*fmt == '%')) {
			++fmt;
			switch (*fmt) {
				case 'c': putchar_cb(*(char *)arg); break;
				case 's': print_cb(*(char **)arg ? *(char **)arg : "(null)"); break;
				case 'b': print_cb(itoa(*(unsigned *)arg, (char*)&toa_result, 2)); break;
				case 'd': print_cb(itoa(*(unsigned *)arg, (char*)&toa_result, 10)); break;
				case 'u': print_cb(utoa(*(unsigned *)arg, (char*)&toa_result, 10)); break;
				case 'y': print_cb(utoa(*(unsigned *)arg, (char*)&toa_result, 16)); break;
				case 'x': print_cb(itoa(*(unsigned *)arg, (char*)&toa_result, 16)); break;
				case 't': print_cb(*(unsigned *)arg ? "yes" : "no"); break;
			}

			if(unlikely(*fmt == '%'))
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
					putchar_cb('s');

			++arg;
		} else putchar_cb(*fmt);
		++fmt;
	}
}

void printf(const char *fmt, ...) {
	vprintf(fmt, (void**)(&fmt) + 1);
}

void serial_printf(const char *fmt, ...) {
	serial_vprintf(fmt, (void**)(&fmt) + 1);
}
