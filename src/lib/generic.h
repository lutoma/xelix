#pragma once

/* Copyright © 2010-2018 Lukas Martini
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

// This file gets included automatically by GCC

#ifndef __xelix__
	#error "Please use a Xelix cross-compiler to compile this code"
#endif

#if __STDC_HOSTED__ != 0
	#error Cannot compile in hosted mode, please use -ffreestanding
#endif

#include <config.h>
#include <stdint.h>
#include "portio.h"
#include <stddef.h>
#include <stdbool.h>
#include <hw/pit.h>

#define POW2(x) (2 << (x - 1))
#define max(a,b) \
	({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	 _a > _b ? _a : _b; })

#define bit_set(num, bit) ((num) | 1 << (bit))
#define bit_clear(num, bit) ((num) & ~(1 << (bit)))
#define bit_toggle(num, bit) ((num) ^ 1 << (bit))
#define bit_get(num, bit) ((num) & (1 << (bit)))

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

#define sleep(t) sleep_ticks((t) * PIT_RATE)

static inline void __attribute__((noreturn)) freeze(void) {
	asm volatile("cli; hlt");
	__builtin_unreachable();
}

static inline void sleep_ticks(time_t timeout) {
	uint64_t start = pit_tick;
	while(pit_tick <= start + timeout);
}

static inline uint32_t uptime(void) {
	return (uint32_t)pit_tick / PIT_RATE;
}

extern void display_clear();
#define clear() display_clear()
