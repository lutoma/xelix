/* frames.h: Control of memory frames
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

#include "frames.h"

#include <common/bitmap.h>
#include <common/multiboot.h>

// 0->free, 1->used. usedFrames->numbits
bitmap_t usedFrames;

void frames_init()
{
	uint32 memSize = multiboot_header->memLower * multiboot_header->memUpper;
	log("frames: memSize: %d\n", memSize);

	// memory bytes / frame size (4kb=0x1000byte)
	usedFrames = bitmap_init(memSize / 0x1000);
	bitmap_clearall(usedFrames); // Free all
}

uint32 frames_allocateFrame()
{
	uint32 frameNum = bitmap_findFirstClearedBit(usedFrames);
	
	if(frameNum == 0 && bitmap_get(usedFrames, 0))
		log("frames: Could not find free frame to allocate! Out of memory!\n");
	
	bitmap_set(usedFrames, frameNum);
	return frameNum;
}

void frames_freeFrame(uint32 frameNum)
{
	if(bitmap_get(usedFrames, frameNum) != 1)
	{
		log("frames: Attempt to free unused frame %d, ignoring.\n", frameNum);
		return;
	}

	bitmap_clear(usedFrames, frameNum);
}
