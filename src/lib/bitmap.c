/* bitmap.c: Bitset / Bitmap implementation
 * Copyright © 2010 Christoph Sünderhauf
 * Copyright © 2011 Lukas Martini
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

#include "bitmap.h"

#include <memory/kmalloc.h>
#include <log.h>
#include <string.h>

/* returns the index of bitmap->bits[] or the offset in the
 * bitmap->bits[index]. Offset such that offset 0 is the lowest bit,
 * offset 7 is the highest bit.
 */
#define index(a) (a/(8*4))
#define offset(a) (a%(8*4))

// returns 1 or 0
uint8_t bitmap_get(bitmap_t bitmap, uint32_t bitnum)
{
	if( bitnum >= bitmap.numbits )
	{
		log(LOG_ERR, "bitmap: bitmap_get() called on a bit number which exceeds the size of the bitmap!");
		return 0;
	}

	if ( bitmap.bits[index(bitnum)] & (0x1 << offset(bitnum)) )
		return 1;
	else
		return 0;
}

// sets a bit (to 1)
void bitmap_set(bitmap_t bitmap, uint32_t bitnum)
{
	if( bitnum >= bitmap.numbits )
	{
		log(LOG_ERR, "bitmap: bitmap_set() called on a bit number which exceeds the size of the bitmap!");
		return;
	}

	bitmap.bits[index(bitnum)] = bitmap.bits[index(bitnum)] | (0x1 << offset(bitnum));
}


// clears a bit (to 0)
void bitmap_clear(bitmap_t bitmap, uint32_t bitnum)
{
	if( bitnum >= bitmap.numbits )
	{
		log(LOG_ERR, "bitmap: bitmap_clear() called on a bit number which exceeds the size of the bitmap!");
		return;
	}

	bitmap.bits[index(bitnum)] = bitmap.bits[index(bitnum)] & ~(0x1 << offset(bitnum));
}

// clears every bit to 0
void bitmap_clearAll(bitmap_t bitmap)
{
	memset(bitmap.bits, 0, sizeof(uint32_t) * (bitmap.numbits-1)/32+1);
}

uint32_t bitmap_findFirstClear(bitmap_t bitmap)
{
	int32_t i;
	for(i=0; i <= index(bitmap.numbits); i++)
	{
		if(bitmap.bits[i] == 0xffffffff)
			continue;

		int j;
		for(j=0; j < 32; j++)
			if(! bitmap_get(bitmap, 32*i+j))
				return 32*i+j;
	}

	// No free bits left
	return 0;
}

bitmap_t bitmap_init(uint32_t numbits)
{
	if(numbits == 0)
	{
		log(LOG_WARN, "bitmap: Empty bitmap requested! Returning NULL.");
		return (bitmap_t){0, NULL};
	}

	bitmap_t bitmap;
	bitmap.numbits = numbits;
	bitmap.bits = (uint32_t*)kmalloc(sizeof(uint32_t) * (numbits-1)/32+1);
	return bitmap;
}
