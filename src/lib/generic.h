#pragma once

/* Copyright © 2010 Lukas Martini, Christoph Sünderhauf
 * Copyright © 2011-2018 Lukas Martini
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

#include <stdint.h>
#include "portio.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __GNUC__
	#define GCC_VERSION (__GNUC__ * 10000 \
								+ __GNUC_MINOR__ * 100 \
								+ __GNUC_PATCHLEVEL__)
#endif

#define ARCH_i386 0
#define ARCH_amd64 1

#define isCharDigit(C) ((C) >= '0' && (C) <= '9')
#define DUMPVAR(C,D) printf("\e[32mdumpvar: %s="C" at %s:%d\e[0m\n", #D, D, __FILE__, __LINE__);

#define POW2(x) (2 << (x - 1))
#define max(a,b) \
	({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	 _a > _b ? _a : _b; })

#define bit_set(num, bit) ((num) | 1 << (bit))
#define bit_clear(num, bit) ((num) & ~(1 << (bit)))
#define bit_toggle(num, bit) ((num) ^ 1 << (bit))
#define bit_get(num, bit) ((num) & (1 << (bit)))

#define bzero(s, n) memset(s, 0, n)

typedef int32_t time_t;
typedef uint8_t byte;

#define EOF  -1

#ifdef __GNUC__
	#define likely(x)       __builtin_expect((x),1)
	#define unlikely(x)     __builtin_expect((x),0)
#else
	#define likely(x) (x)
	#define unlikely(x) (x)
#endif

#define init(C, args...) \
	do \
	{ \
		log(LOG_INFO, "init: Starting to initialize " #C "\n"); \
		C ## _init(args); \
		log(LOG_INFO, "init: Initialized " #C "\n"); \
	} while(0);


static inline void __attribute__((noreturn)) freeze(void) {
	asm volatile("cli; hlt");
	__builtin_unreachable();
}

void memset(void* ptr, uint8_t fill, uint32_t size);
void memcpy(void* dest, void* src, uint32_t size);
char* itoa (int num, char* result, int base);
char* utoa(unsigned int value, char* result, int base);
uint64_t atoi(const char *s);
int32_t memcmp(const void* s1, const void* s2, size_t n);
void sleep(time_t timeout);
void sleep_ticks(time_t timeout);

extern void display_clear();
#define clear() display_clear()
