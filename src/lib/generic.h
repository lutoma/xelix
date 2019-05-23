#pragma once

/* Copyright © 2010-2019 Lukas Martini
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

#if !defined(__i386__)
	#error "Unsupported architecture"
#endif

#ifndef __xelix__
	#error "Please use a Xelix cross-compiler to compile this code"
#endif

#if __STDC_HOSTED__ != 0
	#error Cannot compile in hosted mode, please use -ffreestanding
#endif

#include <config.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/cdefs.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define ALIGN_DOWN(val, to) (typeof(val))((uintptr_t)(val) &~ ((to) - 1))
#define ALIGN(val, to) ((uintptr_t)(val) % (to) \
	? (typeof(val))((intptr_t)ALIGN_DOWN(val, to) + (to)) : (val))

// Symbols provided by LD in linker.ld
extern void* __kernel_start;
extern void* __kernel_end;
#define KERNEL_START ALIGN_DOWN((void*)&__kernel_start, PAGE_SIZE)
#define KERNEL_END ((void*)&__kernel_end)
#define KERNEL_SIZE (KERNEL_END - KERNEL_START)

static inline void __attribute__((noreturn)) freeze(void) {
	asm volatile("cli; hlt");
	__builtin_unreachable();
}

#ifdef __i386__
       #define interrupts_disable() asm volatile("cli")
       #define interrupts_enable() asm volatile("sti")
       #define halt() asm volatile("hlt")
#endif
