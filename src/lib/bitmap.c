/* bitmap.c: Bitmap handling
 * Copyright © 2020-2023 Lukas Martini
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
#include <string.h>
#include <log.h>

#ifdef CONFIG_BITMAP_DEBUG
	#define debug(x, args...) log(LOG_DEBUG, "bitmap: " x, args)
#else
	#define debug(x, args...)
#endif

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
void bitmap_clear_all(struct bitmap* bm) {
	bzero(bm->data, bitmap_size(bm->size));
}

uint32_t bitmap_find(struct bitmap* bm, uint32_t num) {
	int contig_start = 0;
	int contig_num = 0;
	bool all_nonfree = true;

	for(uint32_t i = bm->first_free; i <= bitmap_size(bm->size); i++) {
		uint32_t* bits = &bm->data[i];
		debug("Checking at index %d, value=%#x\n", i, *bits);

		if(*bits == 0xffffffff) {
			debug("All taken\n", i);
			contig_num = 0;
			continue;
		}
		if(all_nonfree) {
			debug("Setting first_free=%d\n", i);
			all_nonfree = false;
			bm->first_free = i;
		}

		if(!*bits) {
			debug("All free\n", i);

			if(!contig_num) {
				contig_start = i*32;
			}
			contig_num += 32;

			if(contig_num >= num) {
				return contig_start;
			}

			continue;
		}

		// Could take some shortcuts here with __builtin_ctz/builtin_clz, but
		// those are tricky to get right. This is good enough for now.

		for(int j = 0; j < 32; j++) {
			debug("Checking bit %d:%d = %d\n", i, j, bit_get(*bits, j) > 0);

			if(bit_get(*bits, j)) {
				contig_num = 0;
			} else {
				if(!contig_num) {
					contig_start = i*32 + j;
				}

				contig_num++;
				debug("contig_start=%d, contig_num=%d.\n", contig_start, contig_num);

				if(contig_num >= num) {
					debug("contig_num is large enough (%d >= %d)\n", contig_num, num);
					return contig_start;
				}
			}
		}
	}

	// No free bits left
	return -1;
}

uint32_t bitmap_get_range(struct bitmap* bm, uint32_t start, uint32_t num) {
	uint32_t remainder = num;
	uint32_t arraypos = start / 32;

	// Deal with potentially unaligned start
	if(start % 32) {
		uint32_t* bits = &bm->data[arraypos];
		uint32_t uaoff = 32 - start % 32;

		if(*bits && __builtin_clz(*bits) < uaoff) {
			return 1;
		}

		remainder -= uaoff;
		arraypos++;
	}

	for(; remainder > 0 && arraypos <= bitmap_size(bm->size); arraypos++) {
		uint32_t* bits = &bm->data[arraypos];

		if(!*bits) {
			if(remainder <= 32) {
				return 0;
			}

			remainder -= 32;
			continue;
		} else {
			if(remainder >= 32) {
				return 1;
			}
		}

		if(__builtin_ctz(*bits) < remainder) {
			return 1;
		}
	}

	return 0;
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
