/* Copyright © 2016-2018 Lukas Martini
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SYS_LIMITS_H
#define _SYS_LIMITS_H

#define PATH_MAX 1024
#define CHAR_BIT 8
#define SCHAR_MIN -127
#define SCHAR_MAX 127
#define UCHAR_MAX 255
#define CHAR_MIN SCHAR_MIN
#define CHAR_MAX SCHAR_MAX
#define MB_LEN_MAX 1
#define SHRT_MIN -32767
#define SHRT_MAX 32767
#define USHRT_MAX 65535
#define INT_MIN -32767
#define INT_MAX 32767
#define UINT_MAX 65535
#define LONG_MIN -2147483647
#define LONG_MAX 2147483647
#define ULONG_MAX 4294967295
#define LLONG_MIN -9223372036854775807
#define LLONG_MAX 9223372036854775807
#define ULLONG_MAX 18446744073709551615

#endif SYS_LIMITS_H
