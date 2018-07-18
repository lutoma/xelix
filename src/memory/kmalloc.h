#pragma once

/* Copyright © 2011 Lukas Martini
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

#include <lib/generic.h>

// Use macros.
void* __attribute__((alloc_size(1))) __kmalloc(size_t sz, bool align, uint32_t *phys, const char* _debug_file, uint32_t _debug_line, const char* _debug_func);

/* A few shortcuts so one doesn't always have to pass all the
 * parameters all the time.
 */
#define kmalloc(sz) __kmalloc(sz, false, NULL, __FILE__, __LINE__, __FUNCTION__)
#define kmalloc_a(sz) __kmalloc(sz, true, NULL, __FILE__, __LINE__, __FUNCTION__)
#define kmalloc_p(sz, phys) __kmalloc(sz, false, phys, __FILE__, __LINE__, __FUNCTION__)
#define kmalloc_ap(sz, phys) __kmalloc(sz, true, phys, __FILE__, __LINE__, __FUNCTION__)
#define kmalloc_pa kmalloc_ap

void kfree(void *ptr);
uint32_t kmalloc_getMemoryPosition();
void kmalloc_init();
