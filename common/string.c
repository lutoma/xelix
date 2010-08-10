#include <common/string.h>
#include <memory/kmalloc.h>


size_t strlen(const char * str)
{
	 const char *s;
	 for (s = str; *s; ++s);
	 return(s - str);
}

char* strcpy(char *dest, const char *src)
{
	char *save = dest;
	while((*dest++ = *src++));
	return save;
}

int strcmp (const char * s1, const char * s2)
{
	 for(; *s1 == *s2; ++s1, ++s2)
			if(*s1 == 0)
				return 0;
	 return *(unsigned char *)s1 < *(unsigned char *)s2 ? -1 : 1;
}

char* strcat(char *dest, const char *src)
{
	 strcpy(dest + strlen(dest), src);
	 return dest;
}

char* substr(char* src, size_t start, size_t len)
{
	char *dest = kmalloc(len+1);
	if (dest) {
		memcpy(dest, src+start, len);
		dest[len] = '\0';
	}
	return dest;
}
