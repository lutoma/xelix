#pragma once

/* Copyright © 2010 Christoph Sünderhauf
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

#define MEMORY_MAX_KMEM 0xBFFFFF


// Use macros.
void* __attribute__((alloc_size(1))) __kmalloc(size_t sz, bool align, uint32_t *phys);

/* A few shortcuts so one doesn't always have to pass all the
 * parameters all the time.
 */ 
#define kmalloc(sz) __kmalloc(sz, false, NULL)
#define kmalloc_a(sz) __kmalloc(sz, true, NULL)
#define kmalloc_p(sz, phys) __kmalloc(sz, false, phys)
#define kmalloc_ap(sz, phys) __kmalloc(sz, true, phys)
#define kmalloc_pa kmalloc_ap

void kfree(void *ptr);
uint32_t kmalloc_getMemoryPosition();
void kmalloc_init();
