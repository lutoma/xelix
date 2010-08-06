#include <common/bitmap.h>
#include <memory/kmalloc.h>



// returns the index of bitmap->bits[] or the offset in the bitmap->bits[index].
// offset such that offset 0 is the lowest bit, offset 7 is the highest bit.
static inline uint32 index(uint32 bitnum)
{
	return bitnum/8;
}
static inline uint32 offset(uint32 bitnum)
{
	return bitnum % 8;
}

bitmap_t* bitmap_init(uint32 numbits)
{
	if(numbits == 0)
	{
		log("Error: bitmap with numbits=0 requested! returning 0");
		return 0;
	}
	bitmap_t* bitmap = kmalloc(sizeof(bitmap));
	bitmap->numbits = kmalloc((numbits-1)/8+1); // (numbits-1)/8 wird abgerundet
	return bitmap;
}

// returns 1 or 0
uint8 bitmap_get(bitmap_t* bitmap, uint32 bitnum)
{
	if( bitnum >= bitmap->numbits )
	{
		log("Error: bitmap_get() called on a bit number which exceeds the size of the bitmap!");
		return 0;
	}
	if ( bitmap->bits[index(bitnum)] & (0x01 << offset(bitnum)) )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

// sets a bit (to 1)
void bitmap_set(bitmap_t* bitmap, uint32 bitnum)
{
	if( bitnum >= bitmap->numbits )
	{
		log("Error: bitmap_get() called on a bit number which exceeds the size of the bitmap!");
		return;
	}
	bitmap->bits[index(bitnum)] = bitmap->bits[index(bitnum)] | (0x01 << offset(bitnum));
}


// clears a bit (to 0)
void bitmap_clear(bitmap_t* bitmap, uint32 bitnum)
{
	if( bitnum >= bitmap->numbits )
	{
		log("Error: bitmap_get() called on a bit number which exceeds the size of the bitmap!");
		return;
	}
	bitmap->bits[index(bitnum)] = bitmap->bits[index(bitnum)] & ~(0x01 << offset(bitnum));
}

// clears every bit to 0
void bitmap_clearall(bitmap_t* bitmap)
{
	memset(bitmap->bits, 0, (bitmap->numbits-1)/8+1); // s.o. bei kmalloc für die Byteanzahl
}
