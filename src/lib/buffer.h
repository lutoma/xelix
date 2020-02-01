#pragma once

/* Copyright © 2020 Lukas Martini
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

#include <spinlock.h>

struct buffer {
	void* data;
	spinlock_t lock;

	// How much data is currently stored
	size_t size;

	// Allocation size (in pages)
	size_t pages;
	size_t max_pages;
};

// Allocate new buffer
struct buffer* buffer_new(size_t max_pages);

// Free buffer
void buffer_free(struct buffer* buf);

// Write to end of buffer
int buffer_write(struct buffer* buf, void* src, size_t size);

// Read from arbitrary location in buffer, leaving content intact
int buffer_read(struct buffer* buf, void* dest, size_t size, size_t offset);

// Read and remove from start of buffer
int buffer_pop(struct buffer* buf, void* dest, size_t size);
