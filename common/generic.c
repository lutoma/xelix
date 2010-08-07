#include <common/generic.h>
#include <devices/display/interface.h>


void memset(void* ptr, uint8 fill, uint32 size)
{
	uint8* p = (uint8*) ptr;
	uint8* max = p+size;
	for(; p < max; p++)
		*p = fill;
}

// Write a byte out to the specified port.
void outb(uint16 port, uint8 value)
{
    asm ("outb %1, %0" : : "dN" (port), "a" (value));
}
void outw(uint16 port, uint16 value)
{
    asm ("outw %1, %0" : : "dN" (value), "a" (port)); // TODO port and value need to be swapped
}

uint8 inb(uint16 port)
{
   uint8 ret;
   asm ("inb %1, %0" : "=a" (ret) : "dN" (port));
   return ret;
}

void printf(char* s)
{
  print(s);
}
void print(char* s)
{
  display_print(s);
}

//Todo: Write to file
void log(char* s)
{
  print(s);
  //if(addn) display_print("\n");
}

void panic(char* reason)
{
	log("\n\nFATAL ERROR: ");
	log(reason);
	for(;;) //Sleep forever
	{
	  asm("cli;hlt;");
	}
}
