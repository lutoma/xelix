#include <common/generic.h>
#include <common/memcpy.h>
#include <devices/display/interface.h>

int logsEnabled;

void memset(void* ptr, uint8 fill, size_t size)
{
	uint8* p = (uint8*) ptr;
	uint8* max = p+size;
	for(; p < max; p++)
		*p = fill;
}

// Write a byte out to the specified port.
void outb(uint16 port, uint8 value)
{
    asm ("outb %1, %0" : : "dN" (port), "a" (value));
}
void outw(uint16 port, uint16 value)
{
    asm ("outw %1, %0" : : "dN" (value), "a" (port)); // TODO port and value need to be swapped
}

uint8 inb(uint16 port)
{
   uint8 ret;
   asm ("inb %1, %0" : "=a" (ret) : "dN" (port));
   return ret;
}

void printf(char* s)
{
  print(s);
}
void print(char* s)
{
  display_print(s);
}

void clear()
{
  display_clear();
}

//Todo: Write to file
void log(char* s)
{
  kernellog = strcat(kernellog, s);
  if(logsEnabled) print(s);
  //if(addn) display_print("\n");
}

void logDec(uint32 num)
{
  if(num == 0)
  {
    log("0");
    return;
  }
  char s[11]; // maximal log(2^(4*8)) (long int sind 4 bytes) + 1 ('\0') = 11
	
  char tmp[9];
  int i=0;
  while(num != 0)
  {
    unsigned char c = num % 10;
    num = (num - c)/10;
    c+='0';
    tmp[i++] = c;
  }
  s[i] = '\0';
  int j;
  for(j=0; j < i; j++)
  {
    s[j] = tmp[i-1-j];
  }
  log(s);
}

void logHex(uint32 num)
{
	if(num == 0)
	{
		log("0x0");
		return;
	}
	char s[11]; // maximal 2 (0x) + 2*4 (long int sind 4 bytes) + 1 ('\0')
	s[0] = '0';
	s[1] = 'x';
	
	char tmp[9];
	int i=0;
	while(num != 0)
	{
		unsigned char c = num & 0xf;
		num = num>>4;
		if(c < 10)
			c+='0';
		else
			c= c-10 + 'A';
		tmp[i++] = c;
	}
	s[i+2] = '\0';
	int j;
	for(j=0; j < i; j++)
	{
		s[2+j] = tmp[i-1-j];
	}
	log(s);
}

void log_init()
{
  kernellog = (char**)kmalloc(4000);
  common_setLogLevel(1); //Enable logs
}

//Currently only 0=Off and 1=On
void common_setLogLevel(int level)
{
  if(level) log("Enabled printing of log messages.\n");
  else log("Warning: disabled printing of log messages.\n");
  logsEnabled = level;
}

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

void panic(char* reason)
{
	asm volatile("cli"); // Disable interrupts.
	log("\n\nFATAL ERROR: ");
	log(reason);
	for(;;) asm("cli;hlt;");//Sleep forever
}

void assert(int r)
{
  if(!r) panic("Assertion failed");
}

char* substr(const char *src, size_t start, size_t len)
{
  char *dest = kmalloc(len+1);
  if (dest) {
    memcpy(dest, src+start, len);
    dest[len] = '\0';
  }
  return dest;
}

/* memcmp */
int (memcmp)(const void *s1, const void *s2, size_t n)
{
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

// dummy
int inw(unsigned int blubb)
{

}
