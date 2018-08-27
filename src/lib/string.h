#pragma once

/* Copyright © 2010 Lukas Martini, Christoph Sünderhauf
 * Copyright © 2011-2018 Lukas Martini
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
#define strcmp __builtin_strcmp
#define strncmp __builtin_strncmp
#define strcat __builtin_strncat
#define strcpy __builtin_strcpy
#define strncpy __builtin_strncpy
#define strlen __builtin_strlen
#define strndup __builtin_strndup
#define memset __builtin_memset
#define memcpy __builtin_memcpy
#define memcmp __builtin_memcmp

char* strtok_r(char* s, const char* delim, char** last);
char* substr(char* src, size_t start, size_t len);
int find_substr(char* list, char* item);
