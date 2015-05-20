/* icmp4.c: Internet Control Message Protocol for IPv4
 * Copyright © 2015 Lukas Martini
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

#include "icmp4.h"
#include <lib/generic.h>
#include <lib/log.h>
#include <lib/endian.h>
#include <net/ip4.h>
#include <memory/kmalloc.h>

void icmp4_send(net_device_t* target, ip4_addr_t src, ip4_addr_t dst,
	uint8_t type, uint8_t code, size_t data_length, void* data)
{
	uint16_t icmp_length = sizeof(icmp4_header_t) + data_length;
	uint16_t packet_length = sizeof(ip4_header_t) + icmp_length;

	ip4_header_t* ip_packet = (ip4_header_t*)kmalloc(packet_length);
	icmp4_header_t* icmp_header = (icmp4_header_t*)((intptr_t)ip_packet +
		sizeof(ip4_header_t));

	ip_packet->src = src;
	ip_packet->dst = dst;
	ip_packet->tos = 0;
	ip_packet->off = 0;
	ip_packet->ttl = 64;
	ip_packet->proto = IP4_TOS_ICMP;
	ip_packet->len = endian_swap16(packet_length);
	ip4_set_header_length(ip_packet, sizeof(ip4_header_t));

	icmp_header->type = type;
	icmp_header->code = code;
	memcpy((intptr_t)icmp_header + sizeof(icmp4_header_t), data, data_length);

	icmp_header->checksum = 0;
	icmp_header->checksum = endian_swap16(net_calculate_checksum((uint8_t*)icmp_header, icmp_length, 0));

	ip4_send(target, packet_length, ip_packet);
}

static void handle_ping(net_device_t* origin, size_t size, ip4_header_t* ip_packet) {
	void* ping_data = (struct echo_data_section*)((intptr_t)ip_packet +
		(ip_packet->hl * 4) + sizeof(icmp4_header_t));

	size_t data_length = size - ip_packet->hl * 4 - sizeof(icmp4_header_t);
	icmp4_send(origin, ip_packet->dst, ip_packet->src, 0, 0, data_length, ping_data);
}

void icmp4_receive(net_device_t* origin, size_t size, ip4_header_t* ip_packet) {
	icmp4_header_t* packet = (icmp4_header_t*)(ip_packet + 1);

	if(packet->type == 8 && packet->code == 0) {
		handle_ping(origin, size, ip_packet);
	}
}
