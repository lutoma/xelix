#include <memory/kmalloc.h>
#include <devices/display/interface.h>

uint32 memoryPosition;
// TODO: improve kmalloc  (heap?)


// simple linear memory allocation without the possibility of free()ing

void* kmalloc(uint32 numbytes)
{
	void* ptr = (void *) memoryPosition;
	memoryPosition += numbytes;
	
	log("Allocated memory at ");
	logHex((int)ptr);
	log("\n");
	
	return ptr;
}


// FIXME: returning physical address only works correctly when paging is disabled.
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
	
	
	log("Allocated aligned memory at ");
	logHex((int)ptr); //fixme
	log("\n");
	
	return ptr;
}

void kmalloc_init(uint32 start)
{
	memoryPosition = (uint32)&start; // maybe put this in an init function?
}
