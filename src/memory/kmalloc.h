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

// vanilla
uint32 kmalloc(size_t sz); 
// page aligned.
uint32 kmalloc_a(size_t sz);
// returns a physical address.
uint32 kmalloc_p(size_t sz, uint32 *phys);
// page aligned and returns a physical address.
uint32 kmalloc_ap(size_t sz, uint32 *phys);

// Make it also work the other way round.
#define kmalloc_pa kmalloc_ap

void kfree(void *ptr);

void kmalloc_init();
