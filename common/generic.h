#ifndef COMMON_H
#define COMMON_H

typedef unsigned int uint32;
typedef signed int sint32;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned char uint8;
typedef signed char sint8;

void memset(void* ptr, uint8 fill, int size);

// Port I/O so that we don't always have to use assembler
void outb(uint16 port, uint8 value);
void outw(uint16 port, uint16 value);
uint8 inb(uint16 port);

void printf(char* s);
void print(char* s);
void log(char* s);
int strcmp(const char *s1, const char *s2);
void panic(char* reason);
void common_setLogLevel(int level);
char *strcat(char *dest, const char *src);
char *strcpy(char *dest, const char *src);
int strlen(const char * str);
void assert(int r);
#endif
