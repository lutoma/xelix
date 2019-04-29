/* serial.c: Driver for i386 serial ports and RPi UART
 * Copyright © 2010 Benjamin Richter
 * Copyright © 2019 Lukas Martini
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

#ifdef __i386__
#define PORT 0x3f8
#define CAN_RECV (inb(PORT+5) & 1)
#define CAN_SEND (inb(PORT+5) & 32)
#endif

#ifdef __arm__
// Controls actuation of pull up/down to ALL GPIO pins.
#define GPPUD 0x94
// Controls actuation of pull up/down for specific GPIO pin.
#define GPPUDCLK0 0x98

#define UART0_BASE 0x1000
#define UART0_DR (UART0_BASE + 0x00)
#define UART0_RSRECR (UART0_BASE + 0x04)
#define UART0_FR (UART0_BASE + 0x18)
#define UART0_ILPR (UART0_BASE + 0x20)
#define UART0_IBRD (UART0_BASE + 0x24)
#define UART0_FBRD (UART0_BASE + 0x28)
#define UART0_LCRH (UART0_BASE + 0x2C)
#define UART0_CR (UART0_BASE + 0x30)
#define UART0_IFLS (UART0_BASE + 0x34)
#define UART0_IMSC (UART0_BASE + 0x38)
#define UART0_RIS (UART0_BASE + 0x3C)
#define UART0_MIS (UART0_BASE + 0x40)
#define UART0_ICR (UART0_BASE + 0x44)
#define UART0_DMACR (UART0_BASE + 0x48)
#define UART0_ITCR (UART0_BASE + 0x80)
#define UART0_ITIP (UART0_BASE + 0x84)
#define UART0_ITOP (UART0_BASE + 0x88)
#define UART0_TDR (UART0_BASE + 0x8C)

// Loop <delay> times in a way that the compiler won't optimize away
static inline void delay(int32_t count) {
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		 : "=r"(count): [count]"0"(count) : "cc");
}
#endif

void serial_send(const char c) {
	#ifdef __i386__
	while(!CAN_SEND);
	outb(PORT, c);
	#else
	while(rpi_mmio_read(UART0_FR) & (1 << 5));
	rpi_mmio_write(UART0_DR, c);
	#endif
}

char serial_recv() {
	#ifdef __i386__
	while(!CAN_RECV);
	return inb(PORT);
	#endif
	return '\0';
}

void serial_print(const char* s) {
	while(*s != '\0')
		serial_send(*(s++));
}

void serial_init() {
	#ifdef __i386__
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
	#elif __arm__
	// Disable UART0.
	rpi_mmio_write(UART0_CR, 0x00000000);
	// Setup the GPIO pin 14 && 15.

	// Disable pull up/down for all GPIO pins
	rpi_mmio_write(GPPUD, 0x00000000);
	delay(150);

	// Disable pull up/down for pin 14,15
	rpi_mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);

	// Write 0 to GPPUDCLK0 to make it take effect.
	rpi_mmio_write(GPPUDCLK0, 0x00000000);

	// Clear pending interrupts.
	rpi_mmio_write(UART0_ICR, 0x7FF);

	// Set integer & fractional part of baud rate.
	// Divider = UART_CLOCK/(16 * Baud)
	// Fraction part register = (Fractional part * 64) + 0.5
	// UART_CLOCK = 3000000; Baud = 115200.

	// Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
	rpi_mmio_write(UART0_IBRD, 1);
	// Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
	rpi_mmio_write(UART0_FBRD, 40);

	// Enable FIFO & 8 bit data transmissio (1 stop bit, no parity).
	rpi_mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

	// Mask all interrupts.
	rpi_mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
	                       (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));

	// Enable UART0, receive & transfer part of UART.
	rpi_mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
	#endif
}
