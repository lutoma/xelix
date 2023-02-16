/* buffer.c: Generic growing fifo buffer
 * Copyright © 2020 Lukas Martini
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

#include <buffer.h>
#include <mem/kmalloc.h>
#include <mem/mem.h>
#include <errno.h>

struct buffer* buffer_new(size_t max_pages) {
	struct buffer* buf = zmalloc(sizeof(struct buffer));
	buf->max_pages = max_pages;
	buf->pages = 1;
	vm_alloc_t vmem;

	if(vm_alloc(VM_KERNEL, &vmem, 1, NULL, VM_RW) != 0) {
		return NULL;
	}

	buf->data = vmem.addr;
	return buf;
}

size_t buffer_write(struct buffer* buf, const void* src, size_t size) {
	if(!spinlock_get(&buf->lock, -1)) {
		return -1;
	}

	if(buf->size + size > buf->pages * PAGE_SIZE) {
		size_t size_needed = buf->pages + RDIV(size, PAGE_SIZE);
		size_t size_wanted = MAX(buf->pages * 3, size_needed);
		size_t size_new = MIN(size_wanted, buf->max_pages);

		if(size_needed > buf->max_pages) {
			sc_errno = EFBIG;
			return -1;
		}

		vm_alloc_t vmem;
		if(vm_alloc(VM_KERNEL, &vmem, size_new, NULL, VM_RW) != 0) {
			return -1;
		}

		memcpy(vmem.addr, buf->data, buf->size);
		pfree((uintptr_t)buf->data / PAGE_SIZE, buf->pages);
		buf->data = vmem.addr;
		buf->pages = size_new;
	}

	memcpy(buf->data + buf->size, src, size);
	buf->size += size;

	spinlock_release(&buf->lock);
	return size;
}

static inline size_t do_read(struct buffer* buf, void* dest, size_t size, size_t offset) {
	if(offset >= buf->size) {
		return 0;
	}

	size = MIN(buf->size - offset, size);
	memcpy(dest, buf->data + offset, size);
	return size;
}

size_t buffer_read(struct buffer* buf, void* dest, size_t size, size_t offset) {
	if(!spinlock_get(&buf->lock, -1)) {
		return -1;
	}

	size_t nread = do_read(buf, dest, size, offset);
	spinlock_release(&buf->lock);
	return nread;
}

size_t buffer_pop(struct buffer* buf, void* dest, size_t size) {
	if(!spinlock_get(&buf->lock, -1)) {
		return -1;
	}

	size_t nread = do_read(buf, dest, size, 0);
	buf->size -= nread;
	if(buf->size) {
		memmove(buf->data, buf->data + size, buf->size);
	}

	spinlock_release(&buf->lock);
	return nread;
}

size_t buffer_size(struct buffer* buf) {
	if(!spinlock_get(&buf->lock, -1)) {
		return -1;
	}

	size_t sz = buf->size;
	spinlock_release(&buf->lock);
	return sz;
}

void buffer_free(struct buffer* buf) {
	pfree((uintptr_t)buf->data / PAGE_SIZE, buf->pages);
	kfree(buf);
}
