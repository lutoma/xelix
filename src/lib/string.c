/* string.c: Common string operations
 * Copyright © 2010 Lukas Martini, Christoph Sünderhauf
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

#include <mem/kmalloc.h>
#include <string.h>
#include <log.h>

size_t strlen(const char* str) {
	const char *s;
	for (s = str; *s != 0 && *s != -1; ++s);
	return(s - str);
}


size_t strnlen(const char *s, size_t maxlen) {
	size_t len;

	for (len = 0; len < maxlen; len++, s++) {
		if (!*s)
			break;
	}
	return (len);
}

char* strcpy(char* dest, const char* src) {
	char* save = dest;
	while((*dest++ = *src++));
	return save;
}

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz) {
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
  }
	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}
	return(s - src - 1);	/* count does not include NUL */
}

char* strncpy(char* dst, const char* src, size_t n) {
	char* p = dst;
	while (n-- && (*dst++ = *src++));
	return p;
}

int strcmp(const char* s1, const char* s2) {
	 for(; *s1 == *s2; ++s1, ++s2)
			if(*s1 == 0)
				return 0;
	 return *(unsigned char *)s1 < *(unsigned char *)s2 ? -1 : 1;
}

int strncmp(const char *s1, const char *s2, register size_t n) {
	register unsigned char u1, u2;
	while (n-- > 0)
		{
			u1 = (unsigned char) *s1++;
			u2 = (unsigned char) *s2++;
			if (u1 != u2)
				return u1 - u2;
			if (u1 == '\0')
				return 0;
		}
	return 0;
}

char* strcat(char *dest, const char *src) {
	 strcpy(dest + strlen(dest), src);
	 return dest;
}

char* substr(char* src, size_t start, size_t len) {
	char *dest = (char*)kmalloc(len+1);
	if (dest) {
		memcpy(dest, src+start, len);
		dest[len] = '\0';
	}
	return dest;
}

char* strtok_r(char* s, const char* delim, char** last) {
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


/* Return index of first match of item in list. */
int find_substr(char* list, char* item) {
	int t;
	char* p, *p2;

	for(t=0; list[t]; t++) {
	p = &list[t];
	p2 = item;

	while(*p2 && *p2==*p) {
		p++;
		p2++;
	}
	if(!*p2) return t; /* 1st return */
	}
	 return -1; /* 2nd return */
}

char* strndup(const char* old, size_t num) {
	char* new = kmalloc(sizeof(char) * (num + 1));
	strlcpy(new, old, num + 1);
	return new;
}

#if !defined(__i386__)
void memset(void* ptr, uint8_t fill, uint32_t size) {
	uint8_t* p = (uint8_t*) ptr;
	uint8_t* max = p+size;

	for(; p < max; p++){
		*p = fill;
	}
}

void* memcpy(void* dest, const void* src, uint32_t size) {
	uint8_t* from = (uint8_t*) src;
	uint8_t* to = (uint8_t*) dest;
	while(size > 0)
	{
		*to = *from;

		size--;
		from++;
		to++;
	}

	return dest;
}
#endif

int32_t memcmp(const void *s1, const void *s2, size_t n) {
	const unsigned char *us1 = (const unsigned char *) s1;
	const unsigned char *us2 = (const unsigned char *) s2;
	while (n-- != 0) {
		if (*us1 != *us2)
			return (*us1 < *us2) ? -1 : +1;

		us1++;
		us2++;
	}
	return 0;
}

void* memmove(void *dst, const void *src, size_t len) {
		size_t i;

		/*
		 * If the buffers don't overlap, it doesn't matter what direction
		 * we copy in. If they do, it does, so just assume they always do.
		 * We don't concern ourselves with the possibility that the region
		 * to copy might roll over across the top of memory, because it's
		 * not going to happen.
		 *
		 * If the destination is above the source, we have to copy
		 * back to front to avoid overwriting the data we want to
		 * copy.
		 *
		 *      dest:       dddddddd
		 *      src:    ssssssss   ^
		 *              |   ^  |___|
		 *              |___|
		 *
		 * If the destination is below the source, we have to copy
		 * front to back.
		 *
		 *      dest:   dddddddd
		 *      src:    ^   ssssssss
		 *              |___|  ^   |
		 *                     |___|
		 */

		if ((uintptr_t)dst < (uintptr_t)src) {
				/*
				 * As author/maintainer of libc, take advantage of the
				 * fact that we know memcpy copies forwards.
				 */
				return memcpy(dst, src, len);
		}

		/*
		 * Copy by words in the common case. Look in memcpy.c for more
		 * information.
		 */

		if ((uintptr_t)dst % sizeof(long) == 0 &&
			(uintptr_t)src % sizeof(long) == 0 &&
			len % sizeof(long) == 0) {

				long *d = dst;
				const long *s = src;

				/*
				 * The reason we copy index i-1 and test i>0 is that
				 * i is unsigned -- so testing i>=0 doesn't work.
				 */

				for (i=len/sizeof(long); i>0; i--) {
						d[i-1] = s[i-1];
				}
		}
		else {
				char *d = dst;
				const char *s = src;

				for (i=len; i>0; i--) {
						d[i-1] = s[i-1];
				}
		}

		return dst;
}

char* strchr(const char *p, int ch) {
	char c;

	c = ch;
	for (;; ++p) {
		if (*p == c)
			return ((char *)p);
		if (*p == '\0')
			return (NULL);
	}
}

char* strrchr(const char *p, int ch) {
	char* save;

	for (save = NULL;; ++p) {
		if (*p == ch)
			save = (char *)p;
		if (!*p)
			return(save);
	}
}

int asprintf(char **strp, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	size_t len = vsnprintf(NULL, 0, fmt, ap) + 1;
	*strp = kmalloc(len);
	size_t read = vsnprintf(*strp, len, fmt, ap);

	va_end(ap);
	return read;
}
