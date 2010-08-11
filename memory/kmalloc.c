#include <memory/kmalloc.h>

// TODO: improve kmalloc  (heap?)


// simple linear memory allocation without the possibility of free()ing


// is defined in the linker script: where the kernel binary stuff ends in memory.
extern uint32 end;
// address (in bytes) where now memory is allocated from.
// It advances an always points to the beginning of the free memory space.
uint32 memoryPosition = (uint32)&end; // maybe put this in an init function?

uint32 kernelMaxMemory = 0xA00000; // 10 megabytes // allocating memory for the kernel won't go beyond this.


void* kmalloc(uint32 numbytes)
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
	kernelMaxMemory = memoryPosition + 0xA00000;
}
