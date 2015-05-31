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

#ifndef _NETINET_IN_H
#define _NETINET_IN_H

#include <stdint.h>

#define IPPROTO_IP 0x0
#define IPPROTO_ICMP 0x1
#define IPPROTO_TCP 0x06
#define IPPROTO_UDP 0x11

#define INADDR_ANY 1
#define INADDR_BROADCAST 2

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr {
	in_addr_t s_addr;
};

struct sockaddr_in {
	short sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
	char sin_zero[8];
};

// Include at the end as it needs some of our typedefs
#include <arpa/inet.h>

#endif /* _NETINET_IN_H */