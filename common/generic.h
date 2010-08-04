#ifndef COMMON_H
#define COMMON_H

typedef unsigned int uint32;
typedef signed int sint32;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned char uint8;
typedef signed char sint8;

// Port i.o. so that we don't always have to use assembler
void outb(uint16 port, uint8 value);
uint8 inb(uint16 port);

void printf(char* s);
void print(char* s);

#endif
