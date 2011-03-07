/* frames.h: Control of memory frames
 * Copyright © 2010 Christoph Sünderhauf
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

#include "frames.h"

#include <common/bitmap.h>
#include <common/generic.h>

// 0->free, 1->used. usedFrames->numbits
bitmap_t usedFrames;

uint32 numFrames;

void frames_init()
{
	numFrames = 0x8000000 / 0x1000; // memory bytes / frame size (4kb=0x1000byte)
	// assume 0x8000000 bytes = 128 Megabytes of memory for the moment.
	usedFrames = bitmap_init(numFrames);

	// set all frames as free
	bitmap_clearall(usedFrames);
}

uint32 frames_allocateFrame()
{
	uint32 frameNum = bitmap_findFirstClearedBit(usedFrames);
	
	if(frameNum == 0 && bitmap_get(usedFrames, 0))
		print("Could not find free frame to allocate! Out of memory!\n");
	
	bitmap_set(usedFrames, frameNum);
	return frameNum;
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
