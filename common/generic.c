#include <common/generic.h>
#include <devices/display/interface.h>

int logsEnabled;

void memset(void* ptr, uint8 fill, int size)
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
  if(logsEnabled) print(s);
  //if(addn) display_print("\n");
}

//Currently only 0=Off and 1=On
void common_setLogLevel(int level)
{
  logsEnabled = level;
}

int strlen(const char * str)
{
    const char *s;
    for (s = str; *s; ++s);
    return(s - str);
}

char* strcpy(char *dest, const char *src)
{
   char *save = dest;
   while((*dest++ = *src++));
   return save;
}

int strcmp (const char * s1, const char * s2)
{
    for(; *s1 == *s2; ++s1, ++s2)
        if(*s1 == 0)
            return 0;
    return *(unsigned char *)s1 < *(unsigned char *)s2 ? -1 : 1;
}

char* strcat(char *dest, const char *src)
{
    strcpy(dest + strlen(dest), src);
    return dest;
}

void panic(char* reason)
{
	asm volatile("cli"); // Disable interrupts.
	log("\n\nFATAL ERROR: ");
	log(reason);
	for(;;) asm("cli;hlt;");//Sleep forever
}

void assert(int r)
{
  if(!r) panic("Assertion failed");
}
