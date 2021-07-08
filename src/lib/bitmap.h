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

#define bit_set(num, bit) ((num) | 1 << (bit))
#define bit_clear(num, bit) ((num) & ~(1 << (bit)))
#define bit_toggle(num, bit) ((num) ^ 1 << (bit))
#define bit_get(num, bit) ((num) & (1 << (bit)))

struct bitmap {
	uint32_t* data;
	uint32_t size;
	int first_free;
};

#define bitmap_index(a) ((a) / (8 * sizeof(uint32_t)))
#define bitmap_offset(a) ((a) % (8 * sizeof(uint32_t)))
#define bitmap_size(size) ((size) + (8 * sizeof(uint32_t)) - 1) / (8 * sizeof(uint32_t))
#define bitmap_get(bm, num) (bit_get((bm)->data[bitmap_index(num)], bitmap_offset(num)))

void bitmap_set(struct bitmap* bm, uint32_t pos, uint32_t num);
void bitmap_clear(struct bitmap* bm, uint32_t pos, uint32_t num);
void bitmap_clear_all(struct bitmap* bm);
uint32_t bitmap_find(struct bitmap* bm, uint32_t num);
uint32_t bitmap_count(struct bitmap* bm);
