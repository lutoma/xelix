/* net.c: Network organization
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

#include "net.h"

#include <lib/log.h>
#include <tasks/scheduler.h>
#include <memory/kmalloc.h>
#include <net/ip4.h>
#include <lib/string.h>
#include <net/slip.h>

#define MAX_DEVICES 51
net_device_t registered_devices[MAX_DEVICES];
uint32_t registered_device_count;

void net_receive(net_device_t* origin, net_proto_t target, size_t size, uint8_t* data)
{
	if(size < 1)
		return;
	
	switch (target)
	{
		case NET_PROTO_IP4:
			ip4_receive(origin, size, data);
		default:
			return;
	}
}

void net_send(net_device_t* target, int origin, size_t size, uint8_t* data)
{
	// Hardcoding for fun and profit
	if(!strcmp(target->driver_name, "slip"))
		slip_send(data, size);
}

void net_register_device(net_device_t* device)
{
	registered_devices[registered_device_count++] = *device;
	kfree(device);
}

// Calculate checksum for TCP, ICMP and IP packets
uint16_t net_calculate_checksum(uint8_t *buf, uint16_t length, uint32_t sum)
{
	// Calculate the sum of 16bit words
	while(length > 1)
	{
		sum += 0xFFFF & (*buf << 8 | *(buf + 1));
		buf += 2;
		length -= 2;
	}

	if(length)
		sum += (0xFF & *buf) << 8;

	while(sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);
	
	// Build 1's complement
	return((uint16_t)sum ^ 0xFFFF);
}

