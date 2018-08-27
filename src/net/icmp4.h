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

#include <net/ip4.h>

#define ICMP4_TYPE_ECHO_REPLY 0
#define ICMP4_TYPE_DEST_UNREACHABLE 3
#define ICMP4_TYPE_SOURCE_QUENCH 4
#define ICMP4_TYPE_ECHO_REQUEST 8
#define ICMP4_TYPE_TIME_EXCEEDED 11
#define ICMP4_TYPE_TRACEROUTE 30

typedef struct {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
} __attribute__((packed)) icmp4_header_t;

void icmp4_receive(net_device_t* origin, size_t size, ip4_header_t* ip_packet);

void icmp4_send(net_device_t* target, ip4_addr_t src, ip4_addr_t dst,
	uint8_t type, uint8_t code, size_t data_length, void* data);

void icmp4_send_error(net_device_t* target, uint8_t type, uint8_t code,
	size_t size, ip4_header_t* packet);