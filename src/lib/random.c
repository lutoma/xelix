/* random.c: Random number generation
 * Copyright © 2019 Lukas Martini
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

#include <fs/sysfs.h>

#define UPPER_MASK 0x80000000
#define LOWER_MASK 0x7fffffff
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define STATE_VECTOR_LENGTH 624
#define STATE_VECTOR_M 397

static uint64_t mt[STATE_VECTOR_LENGTH];
static int index;

void random_seed(uint64_t seed) {
	mt[0] = seed & 0xffffffff;
	for(index=1; index<STATE_VECTOR_LENGTH; index++) {
		mt[index] = (6069 * mt[index-1]) & 0xffffffff;
	}
}

static inline uint64_t getnum() {
	uint64_t y;
	// mag[x] = x * 0x9908b0df for x = 0,1
	static uint64_t mag[2] = {0x0, 0x9908b0df};

	if(index >= STATE_VECTOR_LENGTH || index < 0) {
		// generate STATE_VECTOR_LENGTH words at a time
		int kk;
		if(index >= STATE_VECTOR_LENGTH+1 || index < 0) {
			random_seed(4357);
		}

		for(kk=0; kk<STATE_VECTOR_LENGTH - STATE_VECTOR_M; kk++) {
			y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
			mt[kk] = mt[kk + STATE_VECTOR_M] ^ (y >> 1) ^ mag[y & 0x1];
		}

		for(; kk<STATE_VECTOR_LENGTH - 1; kk++) {
			y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
			mt[kk] = mt[kk+(STATE_VECTOR_M-STATE_VECTOR_LENGTH)] ^ (y >> 1) ^ mag[y & 0x1];
		}

		y = (mt[STATE_VECTOR_LENGTH - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
		mt[STATE_VECTOR_LENGTH - 1] = mt[STATE_VECTOR_M - 1] ^ (y >> 1) ^ mag[y & 0x1];
		index = 0;
	}

	y = mt[index++];
	y ^= (y >> 11);
	y ^= (y << 7) & TEMPERING_MASK_B;
	y ^= (y << 15) & TEMPERING_MASK_C;
	y ^= (y >> 18);
	return y;
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	uint64_t rval = 0;
	uint8_t* rdat = (uint8_t*)&rval;

	for(int i = 0; i < size; i++) {
		int boff = i % sizeof(uint64_t);
		if(!boff) {
			rval = getnum();
		}

		((uint8_t*)dest)[i] = rdat[boff];
	}

	return size;
}

void random_init() {
	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};

	// FIXME /dev/random should be blocking and not rely on the RNG
	sysfs_add_dev("random", &sfs_cb);
	sysfs_add_dev("urandom", &sfs_cb);
}
