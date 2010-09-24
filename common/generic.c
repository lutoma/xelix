/** @file common/generic.c
 * Common utilities often used.
 * @author Lukas Martini
 * @author Christoph Sünderhauf
 */

#include <common/generic.h>
#include <common/string.h>
#include <memory/kmalloc.h>
#include <devices/display/interface.h>

int logsEnabled;
char* kernellog;
uint32 maxLogSize;

/// Memset function. Fills memory with something.
void memset(void* ptr, uint8 fill, uint32 size)
{
	uint8* p = (uint8*) ptr;
	uint8* max = p+size;
	for(; p < max; p++)
		*p = fill;
}
/// Our Memcpy
void memcpy(void* dest, void* src, uint32 size)
{
	uint8* from = (uint8*) src;
	uint8* to = (uint8*) dest;
	while(size > 0)
	{
		*to = *from;
		
		size--;
		from++;
		to++;
	}
}



/** Write out a byte to the specified port
 * @param port The port to write to
 * @param value The valued to be written to the port
 */
void outb(uint16 port, uint8 value)
{
	 asm ("outb %1, %0" : : "dN" (port), "a" (value));
}

/** Write out a word to the specified port
 * @param port The port to write to
 * @param value The valued to be written to the port
 */
void outw(uint16 port, uint16 value)
{
	 asm ("outw %1, %0" : : "dN" (value), "a" (port));
}
/** Read a byte from the specified port
 * @param port The port to read from
 * @return String read from port.
 */
uint8 inb(uint16 port)
{
	uint8 ret;
	asm ("inb %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

/// Print function
void print(char* s)
{
	display_print(s);
}

/// Print int as Hex
void printHex(uint32 num)
{
	display_printHex(num);
}

/// Print int
void printDec(uint32 num)
{
	display_printDec(num);
}

/** Logs something. Also prints it out.
 * @todo Write to file
 * @param s String to log
 */
void log(char* s)
{
	if(strlen(kernellog) + strlen(s) < maxLogSize) // prevent an overflow that is likely to happen if the log gets long enough
		kernellog = strcat(kernellog, s); // concatenate to kernellog
	if(logsEnabled)
		print(s); // print it on screen
}

/** Same as log, only with Integer
 * @todo Write to file
 * @param num Number to log
 */
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

/** Same as log, only with Hex
 * @todo Write to file
 * @param num Number to log
 */
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

/// Initialize log
void log_init()
{
	maxLogSize = 5000;
	kernellog = (char*)kmalloc(maxLogSize);
	kernellog[0] = '\0'; // set kernel log to empty string
	setLogLevel(1); //Enable logs
}

/** Set log level\n
 * @param Log level to be set. Currently only off (0) and on (1).
 */
void setLogLevel(int level)
{
	if(level) log("Enabled printing of log messages.\n");
	else log("Warning: disabled printing of log messages.\n");
	logsEnabled = level;
}

/// Warn. Use the WARN() macro that inserts the line.
void warn(char *reason, char *file, uint32 line)
{
	asm volatile("cli"); // Disable interrupts.
	log("\n\nWARNING: ");
	log(reason);
	log(" in ");
	log(file);
	log(" at line ");
	logDec(line);
	for(;;) asm("cli;hlt;");//Sleep forever
}

/// Panic. Use the PANIC() macro that inserts the line.
void panic(char *reason, char *file, uint32 line, int assertionf)
{
	asm volatile("cli"); // Disable interrupts.
	log("\n\nFATAL ERROR: ");
	if(assertionf) log("Assertion \"");
	log(reason);
	if(assertionf) log("\" failed");
	log(" in ");
	log(file);
	log(" at line ");
	logDec(line);
	for(;;) asm("cli;hlt;");//Sleep forever
}

/// A Memcmp
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

/// Reboot the computer
void reboot()
{
	unsigned char good = 0x02;
	log("Goint to reboot NOW!");
	asm volatile("cli"); //We don't want interrupts here
	while ((good & 0x02) != 0)
	good = inb(0x64);
	outb(0x64, 0xFE);
	//frz();
}
