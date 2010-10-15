// Common utilities often used.

#include <common/generic.h>
#include <common/log.h>
#include <common/string.h>
#include <memory/kmalloc.h>
#include <devices/serial/interface.h>
#include <devices/display/interface.h>

// Memset function. Fills memory with something.
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

// Small helper function
static inline char to_digit(uint8 d)
{
	return (d < 10 ? '0' : 'a' - 10) + d;
}

// A small itoa
// Please note it's not standard c syntax.
char *itoa (int num, int base)
{
	if (num == 0)
		return "0"; 
	
	char buf[8 * sizeof(num) + 1];
	char *res = buf + 8 * sizeof(num);

	*--res = '\0';

	while ((num > 0) && (res >= buf))
	{
		*(--res) = to_digit(num % base);
		num /= base;
	}

	return res;
}


/** Write out a byte to the specified port
 * @param port The port to write to
 * @param value The valued to be written to the port
 */
void outb(uint16 port, uint8 value)
{
	 asm ("outb %1, %0" : : "Nd" (port), "a" (value));
}

/** Write out a word to the specified port
 * @param port The port to write to
 * @param value The valued to be written to the port
 */
void outw(uint16 port, uint16 value)
{
	 asm ("outw %1, %0" : : "Nd" (port), "a" (value));
}
/** Read a byte from the specified port
 * @param port The port to read from
 * @return String read from port.
 */
uint8 inb(uint16 port)
{
	uint8 ret;
	asm ("inb %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

/// Print function
void print(char* s)
{
	serial_print(s);
	display_print(s);
}

void vprintf(const char *fmt, void **arg) {
	while (*fmt) {
		if (*fmt == '%') {
			++fmt;
			switch (*fmt) {
				case 'c': display_printChar(*(char *)arg); break;
				case 's': print(*(char **)arg); break;
				case 'b': print(itoa(*(unsigned *)arg,  2)); break;
				case 'd': print(itoa(*(unsigned *)arg, 10)); break;
				case 'x': print(itoa(*(unsigned *)arg, 16)); break;
			}
			++arg;
		} else display_printChar(*fmt);
		++fmt;
	}
}

void printf(const char *fmt, ...) {
	vprintf(fmt, (void **)(&fmt) + 1);
}

/// Clear screen
void clear(void)
{
	display_clear();
}

/// Warn. Use the WARN() macro that inserts the line.
void warn(char *reason, char *file, uint32 line)
{
	asm volatile("cli"); // Disable interrupts.
	log("\n\nWARNING: %s in %s at line %d", reason, file, line);
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
