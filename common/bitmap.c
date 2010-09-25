#include <common/bitmap.h>
#include <memory/kmalloc.h>



// returns the index of bitmap->bits[] or the offset in the bitmap->bits[index].
// offset such that offset 0 is the lowest bit, offset 7 is the highest bit.
static inline uint32 index(uint32 bitnum)
{
	return bitnum/32;
}
static inline uint32 offset(uint32 bitnum)
{
	return bitnum % 32;
}

bitmap_t bitmap_init(uint32 numbits)
{
	if(numbits == 0)
	{
		log("Error: bitmap with numbits=0 requested! returning 0");
		return (bitmap_t){ 0, NULL };
	}
	bitmap_t bitmap;
	bitmap.numbits = numbits;
	bitmap.bits = kmalloc(sizeof(uint32) * (numbits-1)/32+1); // (numbits-1)/32 wird abgerundet
	return bitmap;
}

// returns 1 or 0
uint8 bitmap_get(bitmap_t bitmap, uint32 bitnum)
{
	if( bitnum >= bitmap.numbits )
	{
		log("Error: bitmap_get() called on a bit number which exceeds the size of the bitmap!");
		return 0;
	}
	if ( bitmap.bits[index(bitnum)] & (0x1 << offset(bitnum)) )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

// sets a bit (to 1)
void bitmap_set(bitmap_t bitmap, uint32 bitnum)
{
	if( bitnum >= bitmap.numbits )
	{
		log("Error: bitmap_get() called on a bit number which exceeds the size of the bitmap!");
		return;
	}
	bitmap.bits[index(bitnum)] = bitmap.bits[index(bitnum)] | (0x1 << offset(bitnum));
}


// clears a bit (to 0)
void bitmap_clear(bitmap_t bitmap, uint32 bitnum)
{
	if( bitnum >= bitmap.numbits )
	{
		log("Error: bitmap_get() called on a bit number which exceeds the size of the bitmap!");
		return;
	}
	bitmap.bits[index(bitnum)] = bitmap.bits[index(bitnum)] & ~(0x1 << offset(bitnum));
}

// clears every bit to 0
void bitmap_clearall(bitmap_t bitmap)
{
	memset(bitmap.bits, 0, sizeof(uint32) * (bitmap.numbits-1)/32+1); // s.o. bei kmalloc für die Byteanzahl
}

uint32 bitmap_findFirstClearedBit(bitmap_t bitmap)
{
	int i;
	for(i=0; i <= index(bitmap.numbits); i++)
	{
		if(bitmap.bits[i] == 0xffffffff)
		{
			continue;
		}
		int j;
		for(j=0; j < 32; j++)
		{
			if(! bitmap_get(bitmap, 32*i+j))
			{
				return 32*i+j;
			}
		}
		
	}
	// error, no free bits left
	return 0;
}
