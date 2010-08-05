#include <common/generic.h>
#include <devices/display/interface.h>

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

void panic(char* reason)
{
	print("\n\nFATAL ERROR: ");
	print(reason);
	for(;;) //Sleep forever
	{
	  asm("cli;hlt;");
	}
}
