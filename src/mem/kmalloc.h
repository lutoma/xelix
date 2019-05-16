#pragma once

/* Copyright © 2011-2019 Lukas Martini
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

#include <stdbool.h>

extern bool kmalloc_ready;

#ifdef KMALLOC_DEBUG
	void* __attribute__((alloc_size(1))) _kmalloc(size_t sz, bool align, bool zero,
		char* _debug_file, uint32_t _debug_line, const char* _debug_func);

	void _kfree(void *ptr, char* _debug_file, uint32_t _debug_line, const char* _debug_func);

	#define kmalloc(sz) _kmalloc(sz, false, false, __FILE__, __LINE__, __FUNCTION__)
	#define kmalloc_a(sz) _kmalloc(sz, true, false, __FILE__, __LINE__, __FUNCTION__)
	#define zmalloc(sz) _kmalloc(sz, false, true, __FILE__, __LINE__, __FUNCTION__)
	#define zmalloc_a(sz) _kmalloc(sz, true, true, __FILE__, __LINE__, __FUNCTION__)
	#define kfree(ptr) _kfree(ptr, __FILE__, __LINE__, __FUNCTION__)
#else
	void* __attribute__((alloc_size(1))) _kmalloc(size_t sz, bool align, bool zero);
	void _kfree(void *ptr);

	#define kmalloc(sz) _kmalloc(sz, false, false)
	#define kmalloc_a(sz) _kmalloc(sz, true, false)
	#define zmalloc(sz) _kmalloc(sz, false, true)
	#define zmalloc_a(sz) _kmalloc(sz, true, true)
	#define kfree(ptr) _kfree(ptr)
#endif

#define kfree_array(arr, max) do { \
	for(int i = 0; i < max && *(arr + i); i++) { \
		kfree(*(arr + i)); \
	} \
	kfree(arr); \
} while(0)

void kmalloc_init();
