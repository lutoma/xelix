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

#include <unistd.h>
// For NULL, which should also be defined in here
#include <stddef.h>

#define EOF -1

// Should work fine for now
typedef void FILE;

// The numbers are defined in unistd.h
#ifdef __INLIBC
	FILE* stdin = (FILE*)STDIN_FILENO;
	FILE* stdout = (FILE*)STDOUT_FILENO;
	FILE* stderr = (FILE*)STDERR_FILENO;
#else
	extern FILE* stdin;
	extern FILE* stdout;
	extern FILE* stderr;
#endif

void print(const char* string);
