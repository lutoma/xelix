/* echo.c: udp/7 echo implementation
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
#include <net/net.h>
#include <net/udp.h>
#include <net/ip4.h>
#include <memory/kmalloc.h>
#include <lib/endian.h>
#include <lib/log.h>

static void echo_server(net_device_t* origin, size_t size, udp_header_t* header, ip4_header_t* ip_packet) {
	uint16_t content_length = endian_swap16(header->length) - sizeof(udp_header_t);
	uint16_t reply_length = sizeof(udp_header_t) + content_length;

	// Construct UDP packet
	udp_header_t* reply = (udp_header_t*)kmalloc(reply_length);
	reply->source_port = header->destination_port;
	reply->destination_port = header->source_port;
	reply->length = endian_swap16(reply_length);
	reply->checksum = 0;
	memcpy(reply + 1, header + 1, content_length);
	reply_length += sizeof(udp_header_t);

	// Construct encasing IPv4 packet
	ip4_header_t* ip_reply = (ip4_header_t*)kmalloc(sizeof(ip4_header_t) + reply_length);
	ip_reply->tos = 0;
	ip_reply->len = endian_swap16((reply_length + sizeof(ip4_header_t)));
	ip_reply->off = 0;
	ip_reply->ttl = 64;
	ip4_set_header_length(ip_reply, sizeof(ip4_header_t));

	ip_reply->src = ip_packet->dst;
	ip_reply->dst = ip_packet->src;

	memcpy((ip_reply + 1), reply, reply_length);
	udp_send(origin, reply_length + sizeof(ip4_header_t), ip_reply);
	kfree(ip_reply);
}

void echo_init() {
	udp_register_handler(echo_server, 7);
}