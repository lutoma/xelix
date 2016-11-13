/* idalloc.c: Internal dumb allocator
 * Copyright © 2016 Lukas Martini
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

#include "idalloc.h"
#include <memory/kmalloc.h>
#include <lib/log.h>
#include <lib/multiboot.h>
#include <hw/serial.h>
#include <lib/panic.h>

/* Internal dumb memory allocator to reduce strain on kmalloc. Does not support
 * partially releasing the memory (only the whole block). Only useful in
 * situations where lots of related small blocks of memory need to be allocated
 * that will not be free'd again individually.
 *
 * Currently used in vmem for the kernel context and for interrupt/page tables.
 */

void* __attribute__((alloc_size(1), malloc)) idalloc(size_t sz, idalloc_ctx_t* ctx) {
	if(!ctx->initialized) {
		#ifdef IDALLOC_DEBUG
			serial_send('I');
		#endif

		ctx->start = (uint32_t)kmalloc(ctx->size);
		ctx->initialized = true;
	}

	if((ctx->pos + sz) > ctx->size) {
		#ifdef IDALLOC_DEBUG
			serial_send('E');
		#endif

		return kmalloc(sz);
	}

	uint32_t addr = ctx->start + ctx->pos;
	ctx->pos += sz;

	return (void*)addr;
}