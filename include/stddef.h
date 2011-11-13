#pragma once

/* Copyright © 2011 Lukas Martini
 *
 * This file is part of Xlibc.
 *
 * Xlibc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Xlibc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Xlibc. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>

#define NULL 0

#ifdef __GNUC__
  #define offsetof(st, m) __builtin_offsetof(st, m)
#else
  #define offsetof(st, m) \
     ((size_t) ( (char *)&((st *)(0))->m - (char *)0 ))
#endif

typedef uint64_t size_t;
typedef uint32_t ptrdiff_t;

/* wchar_t usually is 2 or 4 bytes, but too many doesn't hurt. Change it
 * if you want to.
 */
typedef uint8_t wchar_t;
