/* log.c: Kernel logger
 * Copyright © 2010-2018 Lukas Martini
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <log.h>
#include <print.h>
#include <hw/pit.h>
#include <stdarg.h>

void log(uint32_t level, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);

	#if LOG_SERIAL_LEVEL != 0
	if(level >= LOG_SERIAL_LEVEL) {
		serial_printf("[%d:%d] ", uptime(), pit_tick);
		serial_vprintf(fmt, va);
	}
	#endif

	#if LOG_PRINT_LEVEL != 0
	if(level >= LOG_PRINT_LEVEL) {
		printf("[%d:%d] ", uptime(), pit_tick);
		vprintf(fmt, va);
	}
	#endif

	va_end(va);
}
