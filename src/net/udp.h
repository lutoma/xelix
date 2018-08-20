#pragma once

/* Copyright © 2015 Lukas Martini
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

#include <generic.h>
#include <net/udp.h>
#include <net/ip4.h>
#include <net/net.h>

typedef struct {
	uint16_t source_port;
	uint16_t destination_port;
	uint16_t length;
	uint16_t checksum;
} __attribute__((packed)) udp_header_t;

typedef void (*udp_handler_t)(net_device_t*, size_t, udp_header_t*, ip4_header_t*);

void udp_receive(net_device_t* origin, size_t size, ip4_header_t* ip_packet);
void udp_register_handler(udp_handler_t handler, uint16_t port);
void udp_send(net_device_t* destination, size_t size, ip4_header_t* ip_packet);
void udp_init();