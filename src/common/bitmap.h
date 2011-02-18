#pragma once
#include <common/generic.h>

// provides a bitmap
// can be used as a library throughout the kernel

typedef struct {
	uint32 numbits;
	uint32* bits; // an array large enough for numbits to fit in. Might (if numbits%8!=0) have some spare bits at the end
} bitmap_t;


// creates a new bitmap (using kmalloc). numbits can't be 0.
// CONTENT IS RANDOM!  - use bitmap_clearall() to clear the bitmap.
bitmap_t bitmap_init(uint32 numbits);

// returns 1 or 0
uint8 bitmap_get(bitmap_t bitmap, uint32 bitnum);

// sets a bit (to 1)
void bitmap_set(bitmap_t bitmap, uint32 bitnum);
// clears a bit (to 0)
void bitmap_clear(bitmap_t bitmap, uint32 bitnum);

// clears every bit to 0
void bitmap_clearall(bitmap_t bitmap);

// finds the first bit set to 0    returns 0 if no cleared bit found (0 is also returned if the first bit is cleared)
uint32 bitmap_findFirstClearedBit(bitmap_t bitmap);
