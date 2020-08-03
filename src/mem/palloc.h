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

#include <mem/paging.h>
#include <string.h>

void* palloc(uint32_t num);
void pfree(uint32_t num, uint32_t size);
void palloc_init();
void palloc_get_stats(uint32_t* total, uint32_t* used);

static inline void* zpalloc(uint32_t num) {
	void* buf = palloc(num);
	bzero(buf, num * PAGE_SIZE);
	return buf;
}
