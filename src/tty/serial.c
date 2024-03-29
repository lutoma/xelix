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

#include "serial.h"
#include <portio.h>
#include <fs/sysfs.h>

#define PORT 0x3f8
#define CAN_RECV (inb(PORT+5) & 1)
#define CAN_SEND (inb(PORT+5) & 32)

void serial_send(const char c, void* unused) {
	while(!CAN_SEND) {};
	outb(PORT, c);
}

char serial_recv(void) {
	while(!CAN_RECV);
	return inb(PORT);
}

void serial_init(void) {
	// from http://wiki.osdev.org/Serial_Ports
	// set up with divisor = 3 and 8 data bits, no parity, one stop bit
	// IRQs enabled
	outb(PORT+1, 0x1);
	outb(PORT+3, 0x80);
	outb(PORT+1, 0x00);
	outb(PORT+0, 0x03);
	outb(PORT+3, 0x03);
	outb(PORT+2, 0xC7);
	outb(PORT+4, 0x0B);
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	for(int i = 0; i < size; i++) {
		((char*)dest)[i] = serial_recv();
	}
	return size;
}

static size_t sfs_write(struct vfs_callback_ctx* ctx, void* src, size_t size) {
	for(int i = 0; i < size; i++) {
		serial_send(((char*)src)[i], NULL);
	}
	return size;
}

void serial_init2(void) {
	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
		.write = sfs_write,
	};

	sysfs_add_dev("serial1", &sfs_cb);
}
