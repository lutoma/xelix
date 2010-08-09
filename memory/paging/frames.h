#ifndef MEMORY_PAGING_FRAMES_H
#define MEMORY_PAGING_FRAMES_H
#include <common/generic.h>

void frames_init();

// finds the next free frame (linearly), sets it to used and returns its frame number.
uint32 frames_allocateFrame();


// later we will also need to free a frame (when we have a proper kernel heap or a process dies and its pages are freed)
void frames_freeFrame(uint32 frameNum);

#endif
