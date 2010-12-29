#include <memory/kmalloc.h>

#include <common/log.h>
#define MEMORY_SECTIONS 65536

// TODO: improve search of free memory sections


// is defined in the linker script: where the kernel binary stuff ends in memory.
extern uint32 end;
// address (in bytes) where now memory is allocated from.
// It advances an always points to the beginning of the free memory space.
uint32 memoryPosition = (uint32)&end; // maybe put this in an init function?

uint32 memorySections[MEMORY_SECTIONS];
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
	
	if (nextSection >= MEMORY_SECTIONS)
	{
		if (freeSections != 0)
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
		
		return __kmalloc(numbytes);
	}

	memorySection_t *section = __kmalloc(sizeof(memorySection_t));
	section->free = 0;
	section->size = numbytes;
	memorySections[nextSection] = (uint32)section;
	freeSections--;
	nextSection++;
	
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
}
