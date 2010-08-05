#ifndef MEMORY_KMALLOC_H
#define MEMORY_KMALLOC_H

// usually everything is named with prefix_functions(), except here because this is used so often and is so fundamental.



// returns a pointer to bytes of memory available for usage
void* kmalloc(uint32 bytes);

// dito, but memory aligned to 4kb.
void* kmalloc_aligned(uint32 bytes);

#endif
