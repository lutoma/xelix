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
#include <net/ether.h>
#include <net/ip4.h>
#include <lib/string.h>
#include <net/slip.h>

#define MAX_DEVICES 51
#define MAX_HOSTNAME_LEN 64

net_device_t *registered_devices[MAX_DEVICES];
uint32_t registered_device_count;
char hostname[MAX_HOSTNAME_LEN];
size_t hostname_setlen = 0;

void net_receive(net_device_t* origin, net_l2proto_t proto, size_t size, uint8_t* data)
{
	if(size < 1)
		return;
	
	switch (proto)
	{
		case NET_PROTO_ETH:
			net_ether_receive(origin, data, size);
			break;
		case NET_PROTO_RAW:
			ip4_receive(origin, proto, size, data);
			break;
	}
}

void net_send(net_device_t* target, size_t size, uint8_t* data)
{
	if (target->mtu < size)
	{
		// FIXME Fixme :P
		for (int off = 0; off < size; off += target->mtu)
			net_send(target, target->mtu, data + off);

		return;
	}

	if (target->send != NULL)
		target->send(target, data, size);
}

void net_register_device(net_device_t* device)
{
	registered_devices[registered_device_count++] = device;
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

void net_get_hostname(char* buffer, size_t maxlen)
{
	if(hostname_setlen == 0)
	{
		memcpy(buffer, "(unnamed)", maxlen);
		return;
	}
	
	memcpy(buffer, hostname, maxlen);
}

void net_set_hostname(char* buffer, size_t len)
{
	if(len + 1 > MAX_HOSTNAME_LEN)
		return;
	
	strcpy(hostname, buffer);
	hostname[len] = 0;
	hostname_setlen = len;
	log(LOG_INFO, "net: New hostname: %s\n", hostname);
}
