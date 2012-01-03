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
#include <stdbool.h>

#define EOF -1
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct {
	uint64_t num;
	char filename[512];
	uint32_t offset;
	uint32_t error;
	bool eof;
} FILE;

extern FILE _stdin;
extern FILE _stdout;
extern FILE _stderr;
#define stdin &_stdin
#define stdout &_stdout
#define stderr &_stderr

FILE* fopen(const char* path, const char* mode);
char* fgets(char* str, int num, FILE* fp);
int fputs(const char* string, FILE* fp);
int fseek(FILE* fp, long int offset, int origin);
void clearerr(FILE* fp);

static inline void print(const char* string)
{
        fputs(string, stdout);
}

static inline int ferror(FILE* fp)
{
	return fp->error;
}

static inline int feof(FILE* fp)
{
	return fp->eof;
}