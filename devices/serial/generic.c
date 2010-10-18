// Driver for output over serial port
// Thanks to Waldteufel from whose kernel 'Ambulat' this Code is 'stolen'.

#include <common/generic.h>
#ifdef WITH_SERIAL

#include <devices/serial/interface.h>
#include <common/log.h>

void send(char c);

#define PORT 0x3f8

void serial_init()
{
	// from http://wiki.osdev.org/Serial_Ports
	// set up with divisor = 3 and 8 data bits, no parity, one stop bit

	outb(PORT+1, 0x00); outb(PORT+3, 0x80); outb(PORT+1, 0x00); outb(PORT+0, 0x03);
	outb(PORT+3, 0x03); outb(PORT+2, 0xC7); outb(PORT+4, 0x0B);
}

#define CAN_RECV (inb(PORT+5) & 1)
#define CAN_SEND (inb(PORT+5) & 32)

void send(char c)
{
	while (!CAN_SEND) {};
	outb(PORT, c);
}

char serial_recv()
{
	while (!CAN_RECV) {};
	return inb(PORT);
}

void serial_print(char* s)
{
	while(*s != '\0')
	{
		send(*(s++));
	}
}
#endif
