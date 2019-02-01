/* log.c: Kernel logger
 * Copyright © 2010-2019 Lukas Martini
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
#include <string.h>
#include <fs/sysfs.h>
#include <memory/kmalloc.h>

/* Since the log is also used before kmalloc is initialized, first use a static
 * buffer, then switch as soon as kmalloc is ready.
 */
static char early_buffer[900];
static char* buffer = (char*)&early_buffer;
static size_t buffer_size = 900;
static size_t log_size = 0;

static void store(char* string, size_t len) {
	if(log_size + len > buffer_size) {
		if(!kmalloc_ready) {
			serial_printf("log: Static buffer exhausted before kmalloc is ready.\n");
			return;
		}

		buffer_size *= 2;
		char* new_buffer = (char*)zmalloc(buffer_size);
		memcpy(new_buffer, buffer, log_size);
		if(buffer != (char*)&early_buffer) {
			kfree(buffer);
		}

		buffer = new_buffer;
	}

	memcpy(buffer + log_size, string, len);
	log_size += len;
}

void log(uint32_t level, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);

	char fmt_string[500];
	char prefix[30];
	size_t fmt_len = vsnprintf(fmt_string, 500, fmt, va);
	size_t prefix_len = snprintf(prefix, 30, "[%d:%03d] ", uptime(), pit_tick);

	store(prefix, prefix_len);
	store(fmt_string, fmt_len);

	#if LOG_SERIAL_LEVEL != 0
	if(level >= LOG_SERIAL_LEVEL) {
		serial_printf(prefix);
		serial_printf(fmt_string);
	}
	#endif

	#if LOG_PRINT_LEVEL != 0
	if(level >= LOG_PRINT_LEVEL) {
		printf(prefix);
		printf(fmt_string);
	}
	#endif

	va_end(va);
}

static size_t sfs_read(void* dest, size_t size, size_t offset, void* meta) {
	if(offset >= log_size) {
		return 0;
	}

	if(offset + size > log_size) {
		size = log_size - offset;
	}

	memcpy(dest, buffer + offset, size);
	return size;
}

void log_init() {
	sysfs_add_file("log", sfs_read, NULL, NULL);
}
