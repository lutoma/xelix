/* xelix.c: Implementations for functions in sys/xelix.h
 * Copyright © 2023 Lukas Martini
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

#include <stdio.h>
#include <stdarg.h>
#include <sys/xelix.h>

void _serial_printf(const char* format, ...) {
	if(!_xelix_serial) {
		_xelix_serial = fopen("/dev/serial1", "w");
		setvbuf(_xelix_serial, NULL, _IONBF, 0);
	}

	va_list args;
	va_start (args, format);
	vfprintf(_xelix_serial, format, args);
	va_end (args);
}

__attribute__((destructor)) static void _xelix_serial_cleanup(void) {
	if(_xelix_serial) {
		fclose(_xelix_serial);
	}
}
