#pragma once

/* Copyright © 2010 Lukas Martini, Christoph Sünderhauf
 * Copyright © 2011 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common/stdconf.h>

#define GCC_VERSION (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)

#define isdigit(C) ((C) >= '0' && (C) <= '9')
#define DUMPVAR(C,D) printf("%%dumpvar: %s="C" at %s:%d%%\n", 0x02, #D, D, __FILE__, __LINE__);
#define DUMPVARS(D) DUMPVAR("%s", #D);
#define DUMPVARC(D) DUMPVAR("%c", #D);
#define DUMPVARD(D) DUMPVAR("%d", #D);
#define DUMPVARX(D) DUMPVAR("0x%x", #D);

// Typedefs
typedef unsigned long uint64;
typedef signed long sint64;
typedef unsigned int uint32;
typedef signed int sint32;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned char uint8;
typedef signed char sint8;

typedef long int time_t;
typedef long int size_t;
typedef uint8 byte;
typedef int bool;

#define NULL  0
#define EOF  -1
#define true  1
#define false 0
uint32 i,j; // For counters etc.

bool schedulingEnabled;
#define SPAWN_FUNCTION process_create
#define SPAWN_FUNCTION_N "process_create" //fixme

// Making ponies fly.
#define INIT(C, ...) {\
	log("%%" #C ": Initializing at " __FILE__ ":%d [" #C "_init(" #__VA_ARGS__ ")] (%%", 0x03, __LINE__); \
	if(schedulingEnabled) log("%%" SPAWN_FUNCTION_N ")%%\n", 0x03); \
	else log("%%plain)%%\n", 0x03); \
	if(schedulingEnabled) SPAWN_FUNCTION (#C "_AutoInit", & C ## _init); \
	else C ## _init(__VA_ARGS__); \
	log("%%" #C ": Initialized at " __FILE__ ":%d [" #C "_init(" #__VA_ARGS__ ")] (%%", 0x03, __LINE__); \
	if(schedulingEnabled) log("%%" SPAWN_FUNCTION_N ")%%\n", 0x03); \
	else log("%%plain)%%\n", 0x03); }

// Port I/O so that we don't always have to use assembler
void outb(uint16 port, uint8 value);
void outw(uint16 port, uint16 value);
uint8 inb(uint16 port);
uint8 inbCMOS (uint16 port);
bool init_haveGrub;

// fills size bytes of memory starting at ptr with the byte fill.
void memset(void* ptr, uint8 fill, uint32 size);
// copies size bytes of memory from src to dest
void memcpy(void* dest, void* src, uint32 size); 
//Itoa
char *itoa (int num, int base);

// Printing
void print(char* s);
void vprintf(const char *fmt, void **arg);
void printf(const char *fmt, ...);
void clear(void);

// to automatically have file names and line numbers
//#define WARN(msg) warn(msg, __FILE__, __LINE__);
#define PANIC(msg) panic(msg, __FILE__, __LINE__, 0);
#define ASSERT(b) ((b) ? (void)0 : panic(#b, __FILE__, __LINE__, 1))

// Don't use them, use the macros above.
void panic(char *reason, char *file, uint32 line, int assertionf);
void assert(int r);

// Misc
int (memcmp)(const void *s1, const void *s2, size_t n);
void reboot(); // to be moved later when the halting process becomes more complicated ;)
