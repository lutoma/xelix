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

#include <common/generic.h>

typedef struct {
	uint32 numbits;
	/* an array large enough for numbits to fit in. Might 
	 * (if numbits%8!=0) have some spare bits at the end
	 */
	uint32* bits;
} bitmap_t;


// creates a new bitmap.
// CONTENT IS RANDOM!  - use bitmap_clearall() to clear the bitmap.
bitmap_t bitmap_init(uint32 numbits);

// returns 1 or 0
uint8 bitmap_get(bitmap_t bitmap, uint32 bitnum);
// sets a bit (to 1)
void bitmap_set(bitmap_t bitmap, uint32 bitnum);
// clears a bit (to 0)
void bitmap_clear(bitmap_t bitmap, uint32 bitnum);

// clears every bit to 0
void bitmap_clearAll(bitmap_t bitmap);

// finds the first bit set to 0    returns 0 if no cleared bit found (0 is also returned if the first bit is cleared)
uint32 bitmap_findFirstClear(bitmap_t bitmap);
