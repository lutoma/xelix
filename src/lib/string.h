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
#define strcmp __builtin_strcmp
#define strcasecmp __builtin_strcasecmp
#define strncasecmp __builtin_strncasecmp
#define strncmp __builtin_strncmp
#define strcat __builtin_strcat
#define strcpy __builtin_strcpy
#define strncpy __builtin_strncpy
#define strlen __builtin_strlen
#define strnlen __builtin_strnlen
#define strndup __builtin_strndup
#define memset __builtin_memset
#define memcpy __builtin_memcpy
#define memcmp __builtin_memcmp
#define memmove __builtin_memmove
#define strchr __builtin_strchr

char* strtok_r(char* s, const char* delim, char** last);
char* substr(char* src, size_t start, size_t len);
int find_substr(char* list, char* item);
int asprintf(char **strp, const char *fmt, ...);

static inline void *memset32(uint32_t *s, uint32_t v, size_t n) {
	long d0, d1;
	asm volatile("rep stosl"
		: "=&c" (d0), "=&D" (d1)
		: "a" (v), "1" (s), "0" (n)
		: "memory");
	return s;
}
