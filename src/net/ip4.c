/* ip4.c: Internet Protocol version 4
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

#include "ip4.h"
#include <lib/log.h>
#include <lib/endian.h>
#include <net/ether.h>
#include <memory/kmalloc.h>

void ip4_send(net_device_t* target, size_t size, ip4_header_t* packet);

void ip4_send_ether(net_device_t *target, size_t size, ip4_header_t *packet, ether_frame_hdr_t *hdr)
{
	packet->checksum = 0;
	packet->checksum = endian_swap16(net_calculate_checksum((uint8_t*)packet, sizeof(ip4_header_t), 0));

	if (target->proto != NET_PROTO_ETH)
	{
		ip4_send(target, size, packet);
		return;
	}

	ether_frame_hdr_t *etherhdr = kmalloc(sizeof(ether_frame_hdr_t) + size);
	memset(etherhdr, 0, sizeof(ether_frame_hdr_t));
	if (hdr != NULL)
		memcpy(etherhdr, hdr, sizeof(ether_frame_hdr_t));
	memcpy(etherhdr + 1, packet, size);

	net_send(target, size + sizeof(ether_frame_hdr_t), (uint8_t*)etherhdr);
}

void ip4_send(net_device_t* target, size_t size, ip4_header_t* packet)
{
	//memset((void*)&packet->checksum, 0, sizeof(packet->checksum));
	//packet->checksum = net_calculate_checksum((uint8_t*)packet, 16, 0);
	
	if (target->proto == NET_PROTO_ETH)
	{
		/* TODO Implement some ARP things */
		ip4_send_ether(target, size, packet, NULL);
		return;
	}

	net_send(target, size, (void*)packet);
}

static void handle_icmp(net_device_t* origin, size_t size, ip4_header_t* ip_packet, ether_frame_hdr_t *etherhdr)
{
	ip4_icmp_header_t* packet = (ip4_icmp_header_t*)(ip_packet + 1);
	size_t packet_size = size - sizeof(ip4_header_t);

	//if(packet->type != 8)
	//	return;
	
	if(endian_swap16(packet->sequence) == 1)
		log(LOG_INFO, "net: ip4: 0x%x started ICMP pinging this host.\n", ip_packet->src, endian_swap16(packet->sequence));

	// We can reuse the existing packet as the most stuff stays unmodified
	uint32_t orig_src = ip_packet->src;
	ip_packet->src = ip_packet->dst;
	ip_packet->dst = orig_src;

	packet->type = 0;
	packet->code = 0;

	packet->checksum = 0;
	packet->checksum = endian_swap16(net_calculate_checksum((uint8_t*)packet, packet_size, 0));

	char destination[6];
	memcpy(destination, etherhdr->source, 6);
	memcpy(etherhdr->source, etherhdr->destination, 6);
	memcpy(etherhdr->destination, destination, 6);
	
	ip4_send_ether(origin, size, ip_packet, etherhdr);
}

void ip4_receive(net_device_t* origin, net_l2proto_t proto, size_t size, void* raw)
{
	ip4_header_t *packet = NULL;
	ether_frame_hdr_t *etherhdr = NULL;

	if (proto == NET_PROTO_ETH)
	{
		packet = net_ether_getPayload(raw);
		etherhdr = raw;
		size -= sizeof(ether_frame_hdr_t);
	}
	else if (proto == NET_PROTO_RAW)
		packet = raw;
	// TODO Send an ICMP TTL exceeded packet here
	if(unlikely(packet->ttl <= 0))
		return;
	packet->ttl--;

	if(packet->tos == IP4_TOS_ICMP)
	{
		handle_icmp(origin, size, packet, etherhdr);
		return;
	}
}
