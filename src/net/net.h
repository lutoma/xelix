#pragma once

/* Copyright © 2011 Lukas Martini
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

typedef struct {
	char name[15];
	char driver_name[15];
	char long_driver_name[256];
} net_device_t;

typedef enum {
	NET_PROTO_IP4,
	NET_PROTO_IP6,
	NET_PROTO_ETH,
	NET_PROTO_ARP
} net_proto_t;

void net_receive(net_device_t* origin, net_proto_t target, size_t size, uint8_t* data);
void net_send(net_device_t* target, int origin, size_t size, uint8_t* data);
void net_register_device(net_device_t* device);
uint16_t net_calculate_checksum(uint8_t *buf, uint16_t length, uint32_t sum);
