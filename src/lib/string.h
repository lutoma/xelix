#pragma once

/* Copyright © 2010 Lukas Martini, Christoph Sünderhauf
 * Copyright © 2011-2019 Lukas Martini
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

#include "generic.h"

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)
#define strdup(string) strndup(string, strlen(string))

// string.c
size_t strlen(const char* str);
size_t strnlen(const char *s, size_t maxlen);
char* strcpy(char* dest, const char* src);
size_t strlcpy(char *dst, const char *src, size_t siz);
char* strncpy(char* dst, const char* src, size_t n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char *s1, const char *s2, register size_t n);
char* strcat(char *dest, const char *src);
char* substr(char* src, size_t start, size_t len);
char* strtok_r(char* s, const char* delim, char** last);
int find_substr(char* list, char* item);
char* strndup(const char* old, size_t num);
void memset(void* ptr, uint8_t fill, uint32_t size);
void* memcpy(void* dest, const void* src, uint32_t size);
int32_t memcmp(const void *s1, const void *s2, size_t n);
void* memmove(void *dst, const void *src, size_t len);
char *strchr(const char *p, int ch);
char* strrchr(const char *p, int ch);
int asprintf(char **strp, const char *fmt, ...);

// strcasecmp.c
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

static inline void *memset32(uint32_t *s, uint32_t v, size_t n) {
	long d0, d1;
	asm volatile("rep stosl"
		: "=&c" (d0), "=&D" (d1)
		: "a" (v), "1" (s), "0" (n)
		: "memory");
	return s;
}
