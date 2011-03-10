/* generic.c: Common utilities often used.
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

#include "generic.h"

#include "log.h"
#include "string.h"
#include <memory/kmalloc.h>
#include <devices/serial/interface.h>
#include <devices/display/interface.h>
#include <devices/pit/interface.h>
#include <tasks/scheduler.h>

// Memset function. Fills memory with something.
void memset(void* ptr, uint8 fill, uint32 size)
{
	uint8* p = (uint8*) ptr;
	uint8* max = p+size;
	for(; p < max; p++)
		*p = fill;
}
// Our Memcpy
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
char *itoa(int num, int base)
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

unsigned long atoi(const char *s) {
  unsigned long n = 0;
  while (isdigit(*s)) n = 10 * n + *s++ - '0';
  return n;
}


// Write a byte out to the specified port
void outb(uint16 port, uint8 value)
{
	 asm ("outb %1, %0" : : "Nd" (port), "a" (value));
}

// Write out a word to the specified port
void outw(uint16 port, uint16 value)
{
	 asm ("outw %1, %0" : : "Nd" (port), "a" (value));
}
// Read a byte from the specified port
uint8 inb(uint16 port)
{
	uint8 ret;
	asm ("inb %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}
//Read a byte from the CMOS
uint8 readCMOS (uint16 port)
{
	outb(0x70, port);
	return inb(0x71);
}
//Write a byte into the CMOS
void writeCMOS(uint16 port,uint8 value) {
  uint8 tmp = inb(0x70);
  outb(0x70, (tmp & 0x80) | (port & 0x7F));
  outb(0x71,value);
}


// Print function
void print(char* s)
{
	#ifdef WITH_SERIAL
	serial_print(s);
	#endif
	display_print(s);
}

void vprintf(const char *fmt, void **arg) {
	int state = 0;
	while (*fmt) {
		if (*fmt == '%') {
			++fmt;
			switch (*fmt) {
				case 'c': display_printChar(*(char *)arg); break;
				// Print (null) if pointer == NULL.
				case 's': print(*(char **)arg ? *(char **)arg : "(null)"); break;
				case 'b': print(itoa(*(unsigned *)arg,  2)); break;
				case 'd': print(itoa(*(unsigned *)arg, 10)); break;
				case 'x': print(itoa(*(unsigned *)arg, 16)); break;
				case 't': print(*(unsigned *)arg ? "true" : "false"); break;
			}

			if(*fmt == '%')
			{
				if(!state)
				{
					display_setColor(*(unsigned *)arg);
				} else {
					display_setColor(0x07);
					--arg;
				}
				state = !state;
			}
			
			++arg;
		} else display_printChar(*fmt);
		++fmt;
	}
}

void printf(const char *fmt, ...) {
	vprintf(fmt, (void **)(&fmt) + 1);
}

// Clear screen
void clear(void)
{
	display_clear();
}

// Warn. Use the WARN() macro that inserts the line.
void warn(char *reason, char *file, uint32 line)
{
	asm volatile("cli"); // Disable interrupts.
	log("\n\nWARNING: %s in %s at line %d", reason, file, line);
}

// Panic. Use the PANIC() macro that inserts the line.
void panic(char *file, uint32 line, int assertionf, const char *reason, ...)
{
	// Disable interrupts.
	asm volatile("cli");

	clear();
	printf("%%Kernel Panic!%%\n\n", 0x04);
	printf("Reason: ");
	
	if(assertionf) log("Assertion \"");
	vprintf(reason, (void **)(&reason) + 1);
	if(assertionf) printf("\" failed");
	
	printf("\n");
	printf("The file triggering the kernel panic was %s, line %d.\n\n", file, line);
	
	printf("Last known PIT ticknum: %d\n", pit_getTickNum());
	
	task_t* task = scheduler_getCurrentTask();
	if(task != NULL)
		printf("Last running task PID: %d\n\n", task->pid);
	else
		printf("No task was running or multitasking was disabled.\n\n");
	
	printf("If you can, please tell us what you did when the kernel panic occured.\n");
	printf("Please also make a (manual) screenshot / note the screen output.\n\n");
	
	printf("You can contact us via:\n");
	printf("\tMail: %s",  BUGFIX_MAIL);
	printf("\tIRC: %\n\n",  IRC_CHANNEL);
	
	printf("Thanks!\n\n");
	
	//Sleep forever
	asm("hlt;");
}

// A Memcmp
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

// Reboot the computer
void reboot()
{
	unsigned char good = 0x02;
	log("Going to reboot NOW!");
	asm volatile("cli"); //We don't want interrupts here
	while ((good & 0x02) != 0)
	good = inb(0x64);
	outb(0x64, 0xFE);
	//frz();
}
