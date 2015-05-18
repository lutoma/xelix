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

#include <lib/generic.h>
#include <net/udp.h>
#include <net/ip4.h>
#include <net/net.h>

struct {
	uint16_t source_port;
	uint16_t destination_port;
	uint16_t length;
	uint16_t checksum;
} typedef udp_header_t;

struct {
	uint32_t source_address;
	uint32_t destination_address;
	uint8_t zero;
	uint8_t ip_proto;
	uint16_t udp_length;
} typedef udp_checksum_header_t;

void handle_udp(net_device_t* origin, size_t size, ip4_header_t* ip_packet);