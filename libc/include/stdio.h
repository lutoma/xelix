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
#include <sys/types.h>

#define EOF -1

typedef struct {
  uint64_t num;
  char filename[512];
  uint32_t offset;
} FILE;

extern FILE _stdin;
extern FILE _stdout;
extern FILE _stderr;
#define stdin &_stdin
#define stdout &_stdout
#define stderr &_stderr

char* fgets(char* str, int num, FILE* fp);
int fputs(const char* string, FILE* fp);
static inline void print(const char* string)
{
        fputs(string, stdout);
}

