#pragma once

/* Copyright © 2010 Lukas Martini, Christoph Sünderhauf
 * Copyright © 2011 Lukas Martini
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

// String functions (similiar to the string.h C standard library)
int strcmp(const char *s1, const char *s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcat(char *dest, const char *src);
char* strcpy(char *dest, const char *src);
char* strncpy(char *dst, const char *src, size_t n);
size_t strlen(const char * str);
char* substr(char* src, size_t start, size_t len);
char* strtok(char *s, const char *delim);
char* strtok_r(char *s, const char *delim, char **last);
int find_substr(char *listPointer, char *itemPointer);
char* strndup(const char* old, size_t num);