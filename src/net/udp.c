/* udp.c: User Datagram Protocol
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
#include <net/icmp4.h>
#include <net/net.h>
#include <lib/log.h>
#include <lib/endian.h>
#include <lib/string.h>
#include <memory/kmalloc.h>

// Todo use bitmap?
static udp_handler_t ports[65536];

uint32_t wrapsum(uint32_t sum)
{
	sum = ~sum & 0xFFFF;
	return (endian_swap32(sum));
}

// Todo allow for IP/device-based handler registration
void udp_register_handler(udp_handler_t handler, uint16_t port) {
	ports[port] = handler;
}

void udp_receive(net_device_t* origin, size_t size, ip4_header_t* ip_packet) {
	udp_header_t* header = (udp_header_t*)(ip_packet + 1);
	uint16_t port = endian_swap16(header->destination_port);

	if(port < 1) {
		return;
	}

	if(!ports[port]) {
		icmp4_send_error(origin, ICMP4_TYPE_DEST_UNREACHABLE, 3, size, ip_packet);
		return;
	}
	
	ports[port](origin, size, header, ip_packet);
}

void udp_send(net_device_t* destination, size_t size, ip4_header_t* ip_packet) {
	udp_header_t* header = (udp_header_t*)(ip_packet + 1);
	header->checksum = 0;
	ip_packet->proto = IP4_TOS_UDP;
	ip4_send(destination, size, ip_packet);
}

void udp_init() {
	memset(ports, 0, sizeof(udp_handler_t) * 65536);
}
