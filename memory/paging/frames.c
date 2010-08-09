#include <memory/paging/frames.h>
#include <common/bitmap.h>
#include <common/generic.h>
#include <devices/display/interface.h>

// organises 4 kb frames of the physical memory


// 0->free, 1->used. usedFrames->numbits
bitmap_t* usedFrames;

// The total number of frames. This depends on the amount of memory. (TODO)
uint32 numFrames;



void frames_init()
{
	numFrames = 0x8000000 / 0x1000; // memory bytes / frame size (4kb=0x1000byte)
	// assume 0x8000000 bytes = 128 Megabytes of memory for the moment.
	usedFrames = bitmap_init(numFrames);

	// set all frames as free
	bitmap_clearall(usedFrames);
}


uint32  frames_allocateFrame()
{
	uint32 frameNum;
	for(frameNum=0; 1; frameNum++)
	{
		if(bitmap_get(usedFrames, frameNum) == 0)
		{
			/*print("Allocating frame number ");
			display_printDec(frameNum);
			print("\n");*/
			bitmap_set(usedFrames, frameNum);
			return frameNum;
		}
	}
	print("Could not find free frame to allocate! Out of memory!\n");
	return 0;
}

void frames_freeFrame(uint32 frameNum)
{
	if(bitmap_get(usedFrames, frameNum) != 1)
	{
		print("Trying to free a frame which is not used!\n");
		return;
	}
	bitmap_clear(usedFrames, frameNum);
}
