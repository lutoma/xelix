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
