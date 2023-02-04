/* log.c: Kernel logger
 * Copyright © 2010-2023 Lukas Martini
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
#include <mem/paging.h>

#ifdef CONFIG_LOG_STORE
struct log_entry {
	uint8_t level;
	uint64_t tick;
	uint32_t timestamp;
	uint32_t length;
	char message[];
};

/* Since the log is also used before kmalloc is initialized, first use a static
 * buffer, then switch as soon as kmalloc is ready.
 */
static char early_buffer[PAGE_SIZE];
static void* buffer = (void*)&early_buffer;
static size_t buffer_size = PAGE_SIZE;
static size_t log_size = 0;
static size_t log_entries = 0;

static void store(uint8_t level, char* string, size_t len) {
	size_t new_size = log_size + len + sizeof(struct log_entry);

	if(new_size > buffer_size) {
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

	struct log_entry* entry = (struct log_entry*)((uintptr_t)buffer + log_size);
	entry->level = level;
	entry->tick = timer_tick;
	entry->timestamp = time_get();
	entry->length = len;
	memcpy(entry->message, string, len);
	log_size = new_size;
	log_entries++;
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

void log(uint8_t level, const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);

	char fmt_string[500];
	size_t fmt_len = vsnprintf(fmt_string, 500, fmt, va);
	va_end(va);

	#ifdef CONFIG_LOG_STORE
	/* Only store for log levels > debug. Storing all debug messages is usually
	 * not very helpful, consumes a lot of memory and can cause deadlocks
	 * (kmalloc debug could end up calling kmalloc in store and lock).
	 */
	if(level > LOG_DEBUG) {
		store(level, fmt_string, fmt_len + 1);

		/* Broken at the moment
		if(level == LOG_ERR) {
			ltrace();
		}
		*/
	}
	#endif

	#if CONFIG_LOG_SERIAL_LEVEL != 0
	if(level >= CONFIG_LOG_SERIAL_LEVEL) {
		serial_printf(fmt_string);
	}
	#endif

	#if CONFIG_LOG_PRINT_LEVEL != 0
	if(level >= CONFIG_LOG_PRINT_LEVEL) {
		printf(fmt_string);
	}
	#endif
}

void log_dump() {
	struct log_entry* entry = (struct log_entry*)buffer;
	log(LOG_INFO, "log: Dumping early log, %d entries\n", log_entries);
	for(int i = 0; i < log_entries; i++) {
		printf(entry->message);
		entry = (struct log_entry*)((uintptr_t)entry + entry->length + sizeof(struct log_entry));
	}
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

void log_init() {
	#ifdef CONFIG_LOG_STORE
	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("log", &sfs_cb);
	#endif
}
