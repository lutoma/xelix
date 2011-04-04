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

#ifdef __GNUC__
	#define GCC_VERSION (__GNUC__ * 10000 \
								   + __GNUC_MINOR__ * 100 \
								   + __GNUC_PATCHLEVEL__)

	#define __cdecl __attribute__((__cdecl__))
#endif

#ifndef __cdecl
	// So we can at least compile
	#define __cdecl 
#endif

#define isCharDigit(C) ((C) >= '0' && (C) <= '9')
#define DUMPVAR(C,D) printf("%%dumpvar: %s="C" at %s:%d%%\n", 0x02, #D, D, __FILE__, __LINE__);

// Typedefs
typedef unsigned long uint64;
typedef signed long sint64;
typedef unsigned int uint32;
typedef signed int sint32;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned char uint8;
typedef signed char sint8;

typedef sint64 time_t;
typedef sint64 size_t;
typedef uint8 byte;

typedef enum { false = 0 , true = 1 } bool;

#define NULL  0
#define EOF  -1

void outb(uint16 port, uint8 value);
void outw(uint16 port, uint16 value);
uint8 inb(uint16 port);
void outl(uint16 port, uint32 value);
uint32 inl(uint16 port);
uint8 readCMOS (uint16 port);
void writeCMOS (uint16 port, uint8 value);
void memset(void* ptr, uint8 fill, uint32 size);
void memcpy(void* dest, void* src, uint32 size); 
char* itoa (int num, int base);
void print(char* s);
void vprintf(const char* fmt, void** arg);
void printf(const char* fmt, ...);
void freeze(void);
sint32 memcmp(const void* s1, const void* s2, size_t n);
void reboot();

extern void display_clear();
#define clear() display_clear()

// Don't use this one, use the macros below.
void panic_raw(char *file, uint32 line, const char *reason, ...);

// to automatically have file names and line numbers
#define panic(...) panic_raw( __FILE__, __LINE__, __VA_ARGS__);
#define assert(b) ((b) ? (void)0 : panic_raw(__FILE__, __LINE__, "Assertion \"" #b "\" failed"))
