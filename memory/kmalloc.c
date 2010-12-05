#include <memory/kmalloc.h>
#define MEMORY_SECTIONS 65536

// TODO: improve kmalloc  (heap?)


// simple linear memory allocation without the possibility of free()ing


// is defined in the linker script: where the kernel binary stuff ends in memory.
extern uint32 end;
// address (in bytes) where now memory is allocated from.
// It advances an always points to the beginning of the free memory space.
uint32 memoryPosition = (uint32)&end; // maybe put this in an init function?

uint32 kernelMaxMemory = 0xA00000; // 10 megabytes // allocating memory for the kernel won't go beyond this.

#ifdef WITH_NEW_KMALLOC
uint32 memorySections[MEMORY_SECTIONS];
uint32 nextSection = 0;
#endif

void* __kmalloc(uint32 numbytes)
{
	void* ptr = (void *) memoryPosition;
	memoryPosition += numbytes;

	/*
	print("Allocated ");
	printHex(numbytes);
	print(" bytes at ");
	printHex((int)ptr);
	print(".\n");
	*/
	
	if(memoryPosition >= kernelMaxMemory)
	{
		PANIC("Out of kernel memory");
	}
	
	return ptr;
}

void* kmalloc(uint32 numbytes)
{
  #ifdef WITH_NEW_KMALLOC
  uint32 i = 0;
  while (i < MEMORY_SECTIONS)
  {
    memorySection_t *thisSection = (memorySection_t *)memorySections[i];
    if (thisSection->size == numbytes && thisSection->free != 0)
    {
      thisSection->free = 0;
    
      memset(thisSection->pointer, 0, numbytes);
      
      return thisSection->pointer;
    }
    
    i++;
  }

  memorySection_t *section = __kmalloc(sizeof(memorySection_t));
  section->free = 0;
  section->size = numbytes;
  section->pointer = __kmalloc(numbytes);
  memorySections[nextSection] = (uint32)section;
  nextSection++;
  
  return section->pointer;
  
  #else
  
	return __kmalloc(numbytes);
	#endif
}

#ifdef WITH_NEW_KMALLOC
void kfree(void *ptr)
{
  uint32 i = 0;
  while (i < MEMORY_SECTIONS)
  {
    if ((uint32)(((memorySection_t *)memorySections[i])->pointer) == (uint32) ptr)
    {
      ((memorySection_t *)memorySections[i])->free = 1;
      return;
    }
    i++;
  }
}
#endif

// FIXME: returning physical address only works because of identity paging the kernel heap.
void* kmalloc_aligned(uint32 numbytes, uint32* physicalAddress)
{
	// align to 4 kb (= 0x1000 bytes)
	if( memoryPosition % 0x1000 != 0 )
	{
		memoryPosition = memoryPosition + 0x1000 - (memoryPosition % 0x1000);
	}
	
	void* ptr = (void*) memoryPosition;
	memoryPosition+=numbytes;
	
	if(physicalAddress != 0)
	{
		*physicalAddress = (uint32)ptr;
	}
	
	/*
	print("Allocated ");
	printHex(numbytes);
	print(" bytes of aligned memory at ");
	printHex((int)ptr);
	print(".\n");
	*/
	
	if(memoryPosition >= kernelMaxMemory)
	{
		PANIC("Out of kernel memory");
	}
	
	return ptr;
}

void kmalloc_init(uint32 start)
{
	memoryPosition = (uint32)start;
	kernelMaxMemory = 0x1000 * 1024 * 3 - 1; // a whole number of page tables
}
