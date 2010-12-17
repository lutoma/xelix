#pragma once

#include <common/generic.h>

// usually everything is named with prefix_functions(), except here because this is used so often and is so fundamental, its like a kernel library.


// returns a pointer to numbytes number bytes of memory available for usage
// usage example:
// (int*) var;
// var = (int*) kmalloc(sizeof(int));
// *var = 123;
void* kmalloc(size_t numbytes);

// dito, but memory aligned to 4kb.
// physicalAddress: If physicalAddress is not 0, the physical address of the memory returned is written into that location. The physical address is important when paging is already enabled: Then we can only access memory via its virtual address, but eg. new page directories need to containt physical addresses to their page tables.
void* kmalloc_aligned(size_t numbytes, uint32* physicalAddress);


void kmalloc_init(uint32 start);

#ifdef WITH_NEW_KMALLOC
typedef struct {
	size_t size;
	uint8 free:1;
} memorySection_t;

void kfree(void *ptr);
#endif
