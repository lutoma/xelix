#pragma once

#include <common/stdconf.h>
#include <local.h>

#define GCC_VERSION (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)

#define isdigit(C) ((C) >= '0' && (C) <= '9')

// Typedefs
typedef unsigned int uint32;
typedef signed int sint32;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned char uint8;
typedef signed char sint8;

typedef long int time_t;
typedef long int size_t;
typedef uint8 byte;

#define NULL 0






// Port I/O so that we don't always have to use assembler
void outb(uint16 port, uint8 value);
void outw(uint16 port, uint16 value);
uint8 inb(uint16 port);
uint8 inbCMOS (uint16 port);


// LIB    (a small subset of some c libraries)

// fills size bytes of memory starting at ptr with the byte fill.
void memset(void* ptr, uint8 fill, uint32 size);
// copies size bytes of memory from src to dest
void memcpy(void* dest, void* src, uint32 size); 
//Itoa
char *itoa (int num, int base);

// PRINTING to display
void print(char* s);
void vprintf(const char *fmt, void **arg);
void printf(const char *fmt, ...);
void clear(void);

// to automatically have file names and line numbers
//#define WARN(msg) warn(msg, __FILE__, __LINE__);
#define PANIC(msg) panic(msg, __FILE__, __LINE__, 0);
#define ASSERT(b) ((b) ? (void)0 : panic(#b, __FILE__, __LINE__, 1))

void panic(char *reason, char *file, uint32 line, int assertionf);
void assert(int r);



// MISC

int (memcmp)(const void *s1, const void *s2, size_t n);



void reboot(); // to be moved later when the halting process becomes more complicated ;)
