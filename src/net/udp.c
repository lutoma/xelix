/* ip4.c: User Datagram Protocol
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

#include <lib/generic.h>
#include <net/udp.h>
#include <net/ip4.h>
#include <net/net.h>
#include <lib/log.h>
#include <lib/endian.h>
#include <lib/string.h>
#include <memory/kmalloc.h>

uint32_t wrapsum(uint32_t sum)
{
	sum = ~sum & 0xFFFF;
	return (endian_swap32(sum));
}

void handle_udp(net_device_t* origin, size_t size, ip4_header_t* ip_packet) {
	udp_header_t* header = (udp_header_t*)(ip_packet + 1);
	uint16_t content_length = endian_swap16(header->length) - sizeof(udp_header_t);

	// Simple echo server
	if(endian_swap16(header->destination_port) == 1337) {
		uint16_t reply_length = sizeof(udp_header_t) + content_length;
		udp_header_t* reply = (udp_header_t*)kmalloc(reply_length);
		reply->source_port = header->destination_port;
		reply->destination_port = header->source_port;
		reply->length = endian_swap16(reply_length);
		reply->checksum = 0;

		memcpy(reply + 1, header + 1, content_length);

		// Why do I need to do this crap here?
		reply_length += sizeof(udp_header_t);
		uint16_t udp_length = reply_length;
		ip4_header_t* ip_reply = (ip4_header_t*)kmalloc(sizeof(ip4_header_t) + reply_length);
		ip_reply->hl = ip_packet->hl;
		ip_reply->version = ip_packet->version;
		ip_reply->tos = 0;
		ip_reply->len = endian_swap16(reply_length + sizeof(ip4_header_t));
		ip_reply->id = endian_swap16(1337);
		ip_reply->off = ip_packet->off;
		ip_reply->ttl = 20;
		ip_reply->proto = IP4_TOS_UDP;
	
		ip_reply->src = ip_packet->dst;
		ip_reply->dst = ip_packet->src;
		
		// Now calculate the UDP checksum thingy
		/*udp_checksum_header_t* fake_ip_header = (udp_checksum_header_t*)kmalloc(sizeof(udp_checksum_header_t));
		fake_ip_header->source_address = ip_reply->src;
		fake_ip_header->destination_address = ip_reply->dst;
		fake_ip_header->zero = 0; // How creative
		fake_ip_header->ip_proto = IP4_TOS_UDP;
		fake_ip_header->udp_length = reply->length;

		uint16_t fake_ip_checksum = net_calculate_checksum((uint8_t*)fake_ip_header, sizeof(udp_checksum_header_t), 0);
		uint16_t udp_checksum = net_calculate_checksum((uint8_t*)fake_ip_header, sizeof(udp_checksum_header_t), 0);
		uint16_t udp_data_checksum = net_calculate_checksum((uint8_t*)reply + 1, content_length + sizeof(udp_header_t), 0);
		reply->checksum = wrapsum(fake_ip_checksum + udp_checksum + udp_data_checksum);
*/

		reply->checksum = 0; // Where we're going, we don't need no checksums

		// IP4 checksum
		ip_reply->checksum = 0;
		ip_reply->checksum = endian_swap16(net_calculate_checksum((uint8_t*)ip_reply, reply_length + sizeof(ip4_header_t), 0));

		memcpy((ip_reply + 1), reply, reply_length);
		ip4_send(origin, reply_length + sizeof(ip4_header_t), ip_reply);
	}
}