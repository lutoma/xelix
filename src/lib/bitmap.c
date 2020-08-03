/* bitmap.c: Bitmap handling
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <bitmap.h>
#include <stdbool.h>

void bitmap_set(struct bitmap* bm, uint32_t pos, uint32_t num) {
	for(int i = 0; i < num; i++) {
		uint32_t index = bitmap_index(pos + i);
		uint32_t offset = bitmap_offset(pos + i);

		if(!offset && (num - i) >= 32) {
			bm->data[index] = 0xffffffff;
			i += 31;
		} else {
			bm->data[index] |= 1 << offset;
		}
	}
}

void bitmap_clear(struct bitmap* bm, uint32_t pos, uint32_t num) {
	if(unlikely(pos + num > bm->size)) {
		return;
	}

	for(int i = 0; i < num; i++) {
		uint32_t index = bitmap_index(pos + i);
		uint32_t offset = bitmap_offset(pos + i);

		if(!offset && (num - i) >= 32) {
			bm->data[index] = 0;
			i += 31;
		} else {
			bm->data[index] &= ~(1 << offset);
		}
	}

	bm->first_free = MIN(bm->first_free, pos);
}

uint32_t bitmap_find(struct bitmap* bm, uint32_t num) {
	int contig_start = 0;
	int contig_num = 0;
	bool all_nonfree = true;

	for(uint32_t i = bm->first_free; i <= bitmap_size(bm->size); i++) {
		uint32_t* bits = &bm->data[i];

		if(*bits == 0xffffffff) {
			contig_num = 0;
			continue;
		}
		if(all_nonfree) {
			all_nonfree = false;
			bm->first_free = i;
		}

		if(!*bits) {
			if(!contig_num) {
				contig_start = i*32;
			}
			contig_num += 32;

			if(contig_num + 32 >= num) {
				return contig_start;
			}

			continue;
		}

		int tz = __builtin_ctz(*bits);
		if(contig_num + tz < num) {
			contig_num = 0;
			continue;
		}

		for(int j = 0; j < 32; j++) {
			if(bit_get(*bits, j)) {
				contig_num = 0;
			} else {
				if(!contig_num) {
					contig_start = i*32 + j;
				}
				contig_num++;

				if(contig_num >= num) {
					return contig_start;
				}
			}
		}
	}

	// No free bits left
	return -1;
}

uint32_t bitmap_count(struct bitmap* bm) {
	uint32_t count = 0;
	count += bm->first_free * 32;

	// popcount counts the number of 1 bits in an integer
	for(uint32_t i = bm->first_free; i <= bitmap_size(bm->size); i++) {
		count += __builtin_popcount(bm->data[i]);
	}
	return count;
}
