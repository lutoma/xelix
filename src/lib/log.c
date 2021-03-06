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
#include <printf.h>
#include <bsp/timer.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <panic.h>
#include <fs/sysfs.h>
#include <mem/kmalloc.h>

#ifdef CONFIG_LOG_STORE
/* Since the log is also used before kmalloc is initialized, first use a static
 * buffer, then switch as soon as kmalloc is ready.
 */
static char early_buffer[0x1000];
static char* buffer = (char*)&early_buffer;
static size_t buffer_size = 0x1000;
static size_t log_size = 0;

static void append(char* string, size_t len) {
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

static void store(uint32_t level, char* fmt_string, size_t fmt_len) {
	char log_prefix[100];
	size_t prefix_len = snprintf(log_prefix, 100, "%d %d %d:", timer_tick, time_get(), level);
	append(log_prefix, prefix_len);
	append(fmt_string, fmt_len);
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset >= log_size) {
		return 0;
	}

	if(ctx->fp->offset + size > log_size) {
		size = log_size - ctx->fp->offset;
	}

	memcpy(dest, buffer + ctx->fp->offset, size);
	return size;
}


/*
void  __attribute__((optimize("O0"))) ltrace() {
	intptr_t addresses[10];
	int read = walk_stack(addresses, 10);

	for(int i = 0; i < read; i++) {
		char trace[500];
		size_t trace_len = snprintf(trace, 500, "#%-16d %s <%#x>\n", i, addr2name(addresses[i]), addresses[i]);
		store(LOG_ERR, trace, trace_len);
	}
}
*/
#endif

void log(uint32_t level, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);

	char fmt_string[500];

	/* Only store for log levels > debug. Storing all debug messages is usually
	 * not very helpful, consumes a lot of memory and can cause deadlocks
	 * (kmalloc debug could end up calling kmalloc in store and lock).
	 */
	#ifdef CONFIG_LOG_STORE
	size_t fmt_len = vsnprintf(fmt_string, 500, fmt, va);
	if(level > LOG_DEBUG) {
		store(level, fmt_string, fmt_len);

		/* Broken at the moment
		if(level == LOG_ERR) {
			ltrace();
		}
		*/
	}
	#else
	vsnprintf(fmt_string, 500, fmt, va);
	#endif

	#if CONFIG_LOG_SERIAL_LEVEL != 0 || CONFIG_LOG_PRINT_LEVEL != 0
	char prefix[30];
	snprintf(prefix, 30, "[%d:%03d] ", uptime(), timer_tick);
	#endif

	#if CONFIG_LOG_SERIAL_LEVEL != 0
	if(level >= CONFIG_LOG_SERIAL_LEVEL) {
		serial_printf(prefix);
		serial_printf(fmt_string);
	}
	#endif

	#if CONFIG_LOG_PRINT_LEVEL != 0
	if(level >= CONFIG_LOG_PRINT_LEVEL) {
		printf(prefix);
		printf(fmt_string);
	}
	#endif

	va_end(va);
}

void log_dump() {
	printf(buffer);
}

void log_init() {
	#ifdef CONFIG_LOG_STORE
	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("log", &sfs_cb);
	#endif
}
