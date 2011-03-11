/* serial.c: Driver for most serial ports
 * Copyright © 2010 Benjamin Richter
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

#include <lib/generic.h>
#ifdef WITH_SERIAL
#include "serial.h"

#include <lib/log.h>
#include <lib/datetime.h>

#define PORT 0x3f8
#define CAN_RECV (inb(PORT+5) & 1)
#define CAN_SEND (inb(PORT+5) & 32)

static void send(char c)
{
	while (!CAN_SEND) {};
	outb(PORT, c);
}

char serial_recv()
{
	while (!CAN_RECV)
		sleep(0.0001);

	return inb(PORT);
}

void serial_print(char* s)
{
	while(*s != '\0')
		send(*(s++));
}

void serial_init()
{
	// from http://wiki.osdev.org/Serial_Ports
	// set up with divisor = 3 and 8 data bits, no parity, one stop bit

	outb(PORT+1, 0x00); outb(PORT+3, 0x80); outb(PORT+1, 0x00); outb(PORT+0, 0x03);
	outb(PORT+3, 0x03); outb(PORT+2, 0xC7); outb(PORT+4, 0x0B);
}

#endif
