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
#include <console/interface.h>
#include <memory/kmalloc.h>
#include <hw/serial.h>
#include <hw/display.h>
#include <hw/pit.h>
#include <hw/keyboard.h>
#include <tasks/scheduler.h>
#include <interrupts/interface.h>

#if ARCH == ARCH_i386 || ARCH == ARCH_amd64
	#include <arch/i386/lib/acpi.h>
#endif

// Memset function. Fills memory with something.
void memset(void* ptr, uint8_t fill, uint32_t size)
{
	uint8_t* p = (uint8_t*) ptr;
	uint8_t* max = p+size;
	for(; p < max; p++)
		*p = fill;
}
// Memcpy
void memcpy(void* dest, void* src, uint32_t size)
{
	uint8_t* from = (uint8_t*) src;
	uint8_t* to = (uint8_t*) dest;
	while(size > 0)
	{
		*to = *from;
		
		size--;
		from++;
		to++;
	}
}

// Small helper function
static inline char toDigit(uint8_t d)
{
	return (d < 10 ? '0' : 'a' - 10) + d;
}

// A small itoa
// Please note it's not standard c syntax.
char* itoa(int num, int base)
{
	if (num == 0)
		return "0"; 
	
	char buf[8 * sizeof(num) + 1];
	char *res = buf + 8 * sizeof(num);

	*--res = '\0';

	while ((num > 0) && (res >= buf))
	{
		*(--res) = toDigit(num % base);
		num /= base;
	}

	return res;
}

uint64_t atoi(const char* s) {
  uint64_t n = 0;
  while (isCharDigit(*s)) n = 10 * n + *s++ - '0';
  return n;
}


// Write a byte out to the specified port
void outb(uint16_t port, uint8_t value)
{
	 asm ("out %0, %1" : : "Nd" (port), "a" (value));
}

// Write out a word to the specified port
void outw(uint16_t port, uint16_t value)
{
	 asm ("out %0, %1" : : "Nd" (port), "a" (value));
}
// Read a byte from the specified port
uint8_t inb(uint16_t port)
{
	uint8_t ret;
	asm ("in %0, %1" : "=a" (ret) : "Nd" (port));
	return ret;
}

// Read a word from the specified port
uint16_t inw(uint16_t port)
{
	uint16_t ret;
	asm ("in %0, %1" : "=a" (ret) : "Nd" (port));
	return ret;
}

void outl(uint16_t port, uint32_t value)
{
	 asm ("out %0, %1" : : "Nd" (port), "a" (value));
}

uint32_t inl(uint16_t port)
{
	uint32_t ret;
	asm ("in %0, %1" : "=a" (ret) : "Nd" (port));
	return ret;
}

//Read a byte from the CMOS
uint8_t readCMOS (uint16_t port)
{
	outb(0x70, port);
	return inb(0x71);
}

//Write a byte into the CMOS
void writeCMOS(uint16_t port,uint8_t value) {
  uint8_t tmp = inb(0x70);
  outb(0x70, (tmp & 0x80) | (port & 0x7F));
  outb(0x71,value);
}

// Print function
void print(char* s)
{
	console_write(NULL, s, strlen(s));
}

// Print a single char
static void printChar(char c)
{
	char s[2];
	s[0] = c;
	s[1] = 0;
	
	console_write(NULL, s, 1);
}

// Printing a string, formatted with the stuff in the arg array
void vprintf(const char *fmt, void **arg) {
	bool state = false;
	
	while (*fmt) {
		if (*fmt == '%') {
			++fmt;
			switch (*fmt) {
				case 'c': printChar(*(char *)arg); break;
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
					default_console->info.current_color.foreground = *(unsigned *)arg;
				} else {
					default_console->info.current_color = default_console->info.default_color;
					--arg;
				}
				state = !state;
			}
			
			++arg;
		} else printChar(*fmt);
		++fmt;
	}
}

void printf(const char *fmt, ...) {
	vprintf(fmt, (void **)(&fmt) + 1);
}

/* Freezes the kernel (without possibility to unfreeze).
 * Mainly used for debugging when developing and in panic(_raw).
 */
void freeze(void)
{
	interrupts_disable();
	asm volatile("hlt;");
}

// Panic. Use the panic() macro that inserts the line.
void panic_raw(char *file, uint32_t line, const char *reason, ...)
{
	// Disable interrupts.
	interrupts_disable();

	printf("\n");
	printf("%%Kernel Panic!%%\n\n", 0x04);
	printf("Reason: ");
	
	vprintf(reason, (void **)(&reason) + 1);
	
	printf("\n");
	printf("The file triggering the kernel panic was %s, line %d.\n\n", file, line);
	
	printf("Last known PIT ticknum: %d\n", pit_getTickNum());
	
	task_t* task = scheduler_getCurrentTask();
	/* Can't pass this 1:1 and let printf() do the (null) magic as we've
	 * got a struct here and even tough task may be NULL, with the
	 * offset we could get something printf then shows.
	 */
	
	if(task != NULL)
		printf("Last running task PID: %d\n", task->pid);
	else
		printf("Last running task PID: (null)\n");	
	
	//Sleep forever
	freeze();
}

// A Memcmp
int32_t (memcmp)(const void *s1, const void *s2, size_t n)
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

/* Reboot the computer. Uses the CPU reset function of the keyboard
 * controller. That's right, no tripple faults here. Sorry.
 */
void reboot()
{
	interrupts_disable();
	log(LOG_WARN, "generic: Going to reboot NOW!");
	keyboard_sendKBC(0xFE);
	freeze();
}


void halt()
{
	log(LOG_WARN, "generic: Going to halt NOW!");
	#if ARCH == ARCH_i386 || ARCH == ARCH_amd64
		acpi_powerOff();
	#endif
	
	/* In case we don't have an x86(_64) architecture or the ACPI
	 * poweroff didn't work for whatever reason, display a message
	 * to tell the user he can manually turn of the PC now.
	 */
	printf("\n\nYou may turn off your PC now.\n");
	freeze();
}
