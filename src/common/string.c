/** @file common/string.c
 * \brief Common string operations.
 * @author Lukas Martini
 * @author Christoph Sünderhauf
 * @todo Performance tweaking.
 */

#include <common/string.h>
#include <memory/kmalloc.h>

/** Return length of string
 * @param str String to check length of
 * @return Length of string
 */
size_t strlen(const char * str)
{
	 const char *s;
	 for (s = str; *s; ++s);
	 return(s - str);
}

/** Copy string src to dest
 * @param dest Destination
 * @param src Source
 * @return Destination string
 */
char* strcpy(char *dest, const char *src)
{
	char *save = dest;
	while((*dest++ = *src++));
	return save;
}

// Copy n bytes of src to dst
char *strncpy(char *dst, const char *src, size_t n)
{
	char *p = dst;
	while (n-- && (*dst++ = *src++));
	return p;
}

/** Compare two strings
 * @param s1 First string
 * @param s2 Second string
 * @return 0 if strings are the same, otherwise something else
 */
int strcmp (const char * s1, const char * s2)
{
	 for(; *s1 == *s2; ++s1, ++s2)
			if(*s1 == 0)
				return 0;
	 return *(unsigned char *)s1 < *(unsigned char *)s2 ? -1 : 1;
}

/** Concatenate two strings
 * @param dest First string
 * @param src Second string
 * @return Concatenated string
 */
char* strcat(char *dest, const char *src)
{
	 strcpy(dest + strlen(dest), src);
	 return dest;
}

/** Return part of string
 * @param src Source string
 * @param start Start of returned string
 * @param len Length of returned string
 * @return New string
 */
char* substr(char* src, size_t start, size_t len)
{
	char *dest = kmalloc(len+1);
	if (dest) {
		memcpy(dest, src+start, len);
		dest[len] = '\0';
	}
	return dest;
}

char * strtok(char *s, const char *delim)
{
	static char *last;

	return strtok_r(s, delim, &last);
}

char * strtok_r(char *s, const char *delim, char **last)
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
int find_substr(char *listPointer, char *itemPointer)
{
  int t;
  char *p, *p2;

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
