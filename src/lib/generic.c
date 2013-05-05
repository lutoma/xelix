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
#include "print.h"
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
	
	static char buf[8 * sizeof(num) + 1];
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

/* Freezes the kernel (without possibility to unfreeze).
 * Mainly used for debugging when developing and in panic(_raw).
 */
void freeze(void)
{
	interrupts_disable();
	asm volatile("hlt;");
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

// Sleep x seconds
void sleep(time_t timeout)
{
	timeout *= PIT_RATE;
	timeout++; // Make sure we always wait at least 'timeout'. One tick too long doesn't matter.
	int startTick = pit_getTickNum();
	while(1)
	{
		if(pit_getTickNum() > startTick + timeout) break;
	}
}
