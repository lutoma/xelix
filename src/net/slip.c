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
#include <hw/serial.h>

#define END		0300 // indicates end of packet
#define ESC		0333 // indicates byte stuffing
#define ESC_END	0334 // ESC ESC_END means END data byte
#define ESC_ESC	0335 // ESC ESC_ESC means ESC data byte

size_t slip_recv(uint8_t* buf)
{
	uint8_t d;
	while((d = serial_recv()))
		if(d == END) break;

	log(LOG_DEBUG, "slip: slip_recv: Received packet start sequence.\n");
	
	uint8_t c;
	size_t rlen = 0;
	while(rlen < SLIP_BUFLEN && (c = serial_recv()))
	{	
		switch(c)
		{
			case END:
				buf[rlen] = 0;
				return rlen;
			case ESC:
				c = serial_recv();
				if(c == ESC_END) c = END;
				else if(c == ESC_ESC) c = ESC;
			default:
				buf[rlen++] = c;
		}			
	}
	
	return rlen;
}

void slip_send(uint8_t* buf, size_t len)
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
