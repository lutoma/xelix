#ifndef COMMON_H
#define COMMON_H

typedef unsigned int uint32;
typedef signed int sint32;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned char uint8;
typedef signed char sint8;

// fills size bytes of memory starting at ptr with the byte fill.
void memset(void* ptr, uint8 fill, int size);

// Port i.o. so that we don't always have to use assembler
void outb(uint16 port, uint8 value);
void outw(uint16 port, uint16 value);
uint8 inb(uint16 port);

void printf(char* s);
void print(char* s);
void log(char* s);
void panic(char* reason);
#endif
