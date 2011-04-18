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
 
#include "stdint.h"

#ifndef __GNUC__
	#error It looks like you're trying to compile with a compiler \
!= GNU GCC. Please note that we make heavy use of GCC-specific things \
and therefore, compiling with other compilers won't work. If you want \
to try that anyway, remove this block in src/lib/generic.h.
#endif

#define GCC_VERSION (__GNUC__ * 10000 \
							   + __GNUC_MINOR__ * 100 \
							   + __GNUC_PATCHLEVEL__)

#define ARCH_i386 0
#define ARCH_amd64 1

#define isCharDigit(C) ((C) >= '0' && (C) <= '9')
#define DUMPVAR(C,D) printf("%%dumpvar: %s="C" at %s:%d%%\n", 0x02, #D, D, __FILE__, __LINE__);

typedef int64_t time_t;
typedef int64_t size_t;
typedef uint8_t byte;

typedef enum { false = 0 , true = 1 } bool;

#define NULL  0
#define EOF  -1

// Making ponies fly.
#define init(C, ...) \
	log("%%" #C ": Initializing at " __FILE__ ":%d [" #C "_init(" #__VA_ARGS__ ")] (plain)\n%%", 0x03, __LINE__); \
	C ## _init(__VA_ARGS__); \
	log("%%" #C ": Initialized at " __FILE__ ":%d [" #C "_init(" #__VA_ARGS__ ")] (plain)\n%%", 0x03, __LINE__);

void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
uint8_t inb(uint16_t port);
void outl(uint16_t port, uint32_t value);
uint32_t inl(uint16_t port);
uint8_t readCMOS (uint16_t port);
void writeCMOS (uint16_t port, uint8_t value);
void memset(void* ptr, uint8_t fill, uint32_t size);
void memcpy(void* dest, void* src, uint32_t size); 
char* itoa (int num, int base);
void print(char* s);
void vprintf(const char* fmt, void** arg);
void printf(const char* fmt, ...);
void freeze(void);
int32_t memcmp(const void* s1, const void* s2, size_t n);
void reboot();

extern void display_clear();
#define clear() display_clear()

// Don't use this one, use the macros below.
void panic_raw(char *file, uint32_t line, const char *reason, ...);

// to automatically have file names and line numbers
#define panic(...) panic_raw( __FILE__, __LINE__, __VA_ARGS__)
#define assert(b) do { if(!(b)) panic_raw(__FILE__, __LINE__, "Assertion \"" #b "\" failed."); } while(0);
