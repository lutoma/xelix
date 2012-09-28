#pragma once

/* Copyright © 2012 Lukas Martini
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

#define CHAR_BIT 8
#define CHAR_MIN 0
#define INT_MAX 2147483647
#define LONG_BIT 32
#define LONG_MAX 2147483647
#define MB_LEN_MAX 1
#define SCHAR_MAX 1
#define CHAR_MAX SCHAR_MAX
#define SHRT_MAX 32767
#define UCHAR_MAX 255
#define UINT_MAX 4294967295
#define ULONG_MAX 4294967295
#define SSIZE_MAX UINT_MAX
#define USHRT_MAX 65535
#define WORD_BIT 3
#define INT_MIN -2147483647
#define LONG_MIN -2147483647
#define SCHAR_MIN -128
#define SHRT_MIN -32767
#define LLONG_MIN -9223372036854775807
#define LLONG_MAX 9223372036854775807
#define ULLONG_MAX 18446744073709551615

#define _POSIX_PATH_MAX 256
#define PATH_MAX 512

#define _POSIX_NAME_MAX 256
#define NAME_MAX 512