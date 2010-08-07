#ifndef COMMON_GENERIC_H
#define COMMON_GENERIC_H

typedef unsigned int uint32;
typedef signed int sint32;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned char uint8;
typedef signed char sint8;
typedef long int time_t;
typedef long int size_t;
typedef long long word;  // up to 32 bytes long
typedef uint8 byte;
#define NULL 0
#define wsize sizeof(word)
#define wmask (wsize - 1)

char** kernellog;

// fills size bytes of memory starting at ptr with the byte fill.
void memset(void* ptr, uint8 fill, size_t size);

// Port I/O so that we don't always have to use assembler
void outb(uint16 port, uint8 value);
void outw(uint16 port, uint16 value);
uint8 inb(uint16 port);

void printf(char* s);
void print(char* s);
void clear();
void log(char* s);
void logDec(uint32 num);
void logHex(uint32 num);
void log_init();
int strcmp(const char *s1, const char *s2);
void panic(char* reason);
void common_setLogLevel(int level);
char *strcat(char *dest, const char *src);
char *strcpy(char *dest, const char *src);
size_t strlen(const char * str);
void assert(int r);
char* substr(char** *src, size_t start, size_t len);
int (memcmp)(const void *s1, const void *s2, size_t n);
int inw(unsigned int blubb);
void reboot();
#endif
