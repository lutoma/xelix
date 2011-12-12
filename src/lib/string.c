/* string.c: Common string operations
 * Copyright © 2010 Lukas Martini, Christoph Sünderhauf
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

#include "string.h"

#include <memory/kmalloc.h>
#include <lib/log.h>

// Return the length of a string
size_t strlen(const char* str)
{
	 const char *s;
	 for (s = str; *s; ++s);
	 return(s - str);
}

char* strcpy(char* dest, const char* src)
{
	char* save = dest;
	while((*dest++ = *src++));
	return save;
}

// Copy n bytes of src to dst
char* strncpy(char* dst, const char* src, size_t n)
{
	char* p = dst;
	while (n-- && (*dst++ = *src++));
	return p;
}

// Compare two strings
int strcmp(const char* s1, const char* s2)
{
	 for(; *s1 == *s2; ++s1, ++s2)
			if(*s1 == 0)
				return 0;
	 return *(unsigned char *)s1 < *(unsigned char *)s2 ? -1 : 1;
}

// compare two strings without fearing buffer overflows :p
int strncmp(const char* s1, const char* s2, size_t n)
{
    if (n == 0) return 0;
    do {
        if (*s1 < *s2)
            return -1;
        if (*s1 > *s2)
            return 1;
        s1++; s2++;
    } while (n--);
    return 0;
}


// Concatenate two strings
char* strcat(char *dest, const char *src)
{
	 strcpy(dest + strlen(dest), src);
	 return dest;
}

// Return part of string
char* substr(char* src, size_t start, size_t len)
{
	char *dest = (char*)kmalloc(len+1);
	if (dest) {
		memcpy(dest, src+start, len);
		dest[len] = '\0';
	}
	return dest;
}

char* strtok(char *s, const char *delim)
{
	log(LOG_WARN, "string: The usage of strtok is deprecated and dangerous. Please use strtok_r.\n");
	static char *last;
	return strtok_r(s, delim, &last);
}

char* strtok_r(char* s, const char* delim, char** last)
{
	char *spanp;
	int c, sc;
	char *tok;

	if (s == NULL && (s = *last) == NULL)
		return (NULL);

	/*
	 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
	 */
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		*last = NULL;
		return (NULL);
	}
	tok = s - 1;

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*last = s;
				return (tok);
			}
		} while (sc != 0);
	}
}


/* Return index of first match of itemPointer in listPointer. */
int find_substr(char* listPointer, char* itemPointer)
{
  int t;
  char* p, *p2;

  for(t=0; listPointer[t]; t++) {
    p = &listPointer[t];
    p2 = itemPointer;

    while(*p2 && *p2==*p) {
      p++;
      p2++;
    }
    if(!*p2) return t; /* 1st return */
  }
   return -1; /* 2nd return */
}
