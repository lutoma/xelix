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
#include <stdlib.h>
 #include <stdarg.h>

#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// Buffering settings
#define _IOFBF 0 // Full
#define _IOLBF 1 // Line
#define _IONBF 2 // Disabled

typedef struct {
	uint64_t num;
	char filename[512];
	uint32_t offset;
	uint32_t error;
	int eof;
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

static inline FILE* tmpfile()
{
	return NULL;
}


// printf-family
extern int snprintf(char *, size_t, const char *, /*args*/ ...);
extern int vsnprintf(char *, size_t, const char *, va_list);
extern int asprintf  (char **ptr, const char *fmt, /*args*/ ...);
extern int vasprintf (char **ptr, const char *fmt, va_list ap);
extern int asnprintf (char **ptr, size_t str_m, const char *fmt, /*args*/ ...);
extern int vasnprintf(char **ptr, size_t str_m, const char *fmt, va_list ap);
#undef printf
int printf(const char *format, ...);
int sprintf(char *out, const char *format, ...);

static inline void vfprintf(FILE* fp, const char *fmt, void** arg)
{
	return;
}

static inline void vprintf(const char *fmt, void** arg)
{
	vfprintf(stdout, fmt, arg);
}


static inline void fprintf(FILE* fp, const char *fmt, ...) {
	vfprintf(fp, fmt, (void**)(&fmt) + 1);
}

// printf end

static inline int putc(int c, FILE* fd)
{
	char* s = (char*)malloc(2 * sizeof(int));
	s[0] = c;
	s[1] = 0;
	fputs(s, fd);
	free(s);
	return 0;
}

static inline int putchar(int c)
{
	return putc(c, stdout);
}

static inline int ferror(FILE* fp)
{
	return fp->error;
}

static inline int feof(FILE* fp)
{
	return fp->eof;
}