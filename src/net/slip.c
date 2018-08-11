/* slip.c: IP over Serial Lines (RFC 1055)
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

// TODO Convert 0300 and 0333 in data stream to their escape codes

#include "slip.h"

#include <lib/log.h>
#include <memory/kmalloc.h>
#include <hw/interrupts.h>
#include <hw/serial.h>
#include <net/net.h>

#define BUFSIZE	1006
#define END		0xc0 // indicates end of packet
#define ESC		0xdb // indicates byte stuffing
#define ESC_END	0xdc // ESC ESC_END means END data byte
#define ESC_ESC	0xdd // ESC ESC_ESC means ESC data byte

static net_device_t* mydev;
static uint8_t* buf;
static bool in_progress = false;
static int bufpos = 0;

void slip_receive(cpu_state_t* state)
{
	uint8_t c = serial_recv();

	if(!in_progress)
	{
		if(c != END)
			return;

		buf = (uint8_t*)kmalloc(sizeof(uint8_t) * BUFSIZE);
		in_progress = true;
		return;
	}

	if((c == END && in_progress) || bufpos >= BUFSIZE)
	{
		in_progress = false;
		// TODO IPv6 support
		net_receive(mydev, NET_PROTO_RAW, bufpos, buf);
		bufpos = -1;
	}

	switch(c)
	{
		case ESC:
			c = serial_recv();
			if(c == ESC_END) c = END;
			else if(c == ESC_ESC) c = ESC;
		default:
			buf[bufpos++] = c;
	}
}

static void slip_send(net_device_t *dev, uint8_t* buf, size_t len)
{
	serial_send(END);

	uint8_t* in = buf;
	for(; len; in++, len--)
	{
		switch(*in)
		{
			case END:
				serial_send(ESC);
				serial_send(ESC_END);
				break;
			case ESC:
				serial_send(ESC);
				serial_send(ESC_ESC);
				break;
			default:
				serial_send(*in);
		}
	}

	serial_send(END);
}

void slip_init()
{
	mydev = (net_device_t*)kmalloc(sizeof(net_device_t));
	memcpy(mydev->name, "slip0", 6);
	mydev->proto = NET_PROTO_RAW;
	mydev->mtu = 65535;
	mydev->send = slip_send;
	net_register_device(mydev);

	// Hook up to the serial port #1 interrupt
	interrupts_register(IRQ4, slip_receive);
}
