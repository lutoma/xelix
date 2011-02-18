#include <memory/kmalloc.h>
#include <common/log.h>
#define MEMORY_SECTIONS 65536

// TODO: improve search of free memory sections

uint32 memoryPosition;
uint32 *memorySections = NULL;
uint32 freeSections = MEMORY_SECTIONS;
uint32 nextSection = 0;

void* __kmalloc(size_t numbytes)
{
	void* ptr = (void *) memoryPosition;
	memoryPosition += numbytes;

	if(memoryPosition >= MEMORY_MAX_KMEM)
		PANIC("Out of kernel memory");
	
	return ptr;
}

void* kmalloc(size_t numbytes)
{
	if (freeSections == 0 || memorySections == NULL) return __kmalloc(numbytes);

	if (freeSections < MEMORY_SECTIONS)
	{
		uint32 i = 0;
		while (i < nextSection)
		{
			memorySection_t *thisSection = (memorySection_t *)memorySections[i];
			if (thisSection->size == numbytes && thisSection->free != 0)
			{
				thisSection->free = 0;
				void *pointer = (void*)((uint32)thisSection + sizeof(memorySection_t));
				freeSections--;
				return pointer;
			}

			i++;
		}
	}
	
	if (nextSection >= MEMORY_SECTIONS)
	{
		i = 0;
		while (i < nextSection)
		{
			memorySection_t *thisSection = (memorySection_t *)memorySections[i];
			if (thisSection->free != 0 && thisSection->size > numbytes)
			{
				thisSection->free = 0;
				thisSection->size = numbytes;
				void *pointer = (void*)((uint32)thisSection + sizeof(memorySection_t));
				freeSections--;
				return pointer;
			}
		}
	}
	else
	{
		memorySection_t *section = __kmalloc(sizeof(memorySection_t));
		section->free = 0;
		section->size = numbytes;
		memorySections[nextSection] = (uint32)section;
		freeSections--;
		nextSection++;
	}
	
	return __kmalloc(numbytes);
}


void kfree(void *ptr)
{
	uint32 i = 0;
	while (i < nextSection)
	{
		uint32 section_pointer = memorySections[i] + sizeof(memorySection_t);
		if (section_pointer == (uint32) ptr)
		{
			((memorySection_t *)memorySections[i])->free = 1;
			freeSections++;
			return;
		}

		i++;
	}
}


// FIXME: returning physical address only works because of identity paging the kernel heap.
void* kmalloc_aligned(size_t numbytes, uint32* physicalAddress)
{
	// align to 4 kb (= 0x1000 bytes)
	if( memoryPosition % 0x1000 != 0 )
		memoryPosition = memoryPosition + 0x1000 - (memoryPosition % 0x1000);
	
	void* ptr = (void*) memoryPosition;
	memoryPosition+=numbytes;
	
	if(physicalAddress != 0)
		*physicalAddress = (uint32)ptr;
	
	if(memoryPosition >= MEMORY_MAX_KMEM)
		PANIC("Out of kernel memory");
	
	return ptr;
}

void kmalloc_init(uint32 start)
{
	memoryPosition = (uint32)start;
	memorySections = __kmalloc(sizeof(uint32) * MEMORY_SECTIONS);
}
