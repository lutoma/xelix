/* conv.c: BSD/PicoTCP port/saddr Conversions
 * Copyright © 2013 TASS Belgium NV
 * Copyright © 2019 Lukas Martini
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <net/conv.h>
#include <net/socket.h>
#include <pico_socket.h>
#include <errno.h>

int net_bsd_to_pico_addr(union pico_address *addr, const struct sockaddr *_saddr, socklen_t socklen) {
	if(socklen != SOCKSIZE && socklen != SOCKSIZE6) {
		return -1;
	}

	if (socklen == SOCKSIZE6) {
		const struct sockaddr_in6 *saddr = (const struct sockaddr_in6 *)_saddr;
		memcpy(&addr->ip6.addr, &saddr->sin6_addr.s6_addr, 16);
	} else {
		const struct sockaddr_in *saddr = (const struct sockaddr_in *)_saddr;
		addr->ip4.addr = saddr->sin_addr.s_addr;
	}
	return 0;
}

uint16_t net_bsd_to_pico_port(const struct sockaddr *_saddr, socklen_t socklen) {
	if(socklen != SOCKSIZE && socklen != SOCKSIZE6) {
		return -1;
	}

	if (socklen == SOCKSIZE6) {
		const struct sockaddr_in6 *saddr = (const struct sockaddr_in6 *)_saddr;
		return saddr->sin6_port;
	} else {
		const struct sockaddr_in *saddr = (const struct sockaddr_in *)_saddr;
		return saddr->sin_port;
	}
}

int net_conv_pico2bsd(struct sockaddr* saddr, socklen_t socklen, union pico_address* addr, uint16_t port) {
	struct sockaddr_in6* saddr6 = (struct sockaddr_in6*)saddr;
	struct sockaddr_in* saddr4 = (struct sockaddr_in*)saddr;

	switch(socklen) {
		case SOCKSIZE6:
			memcpy(&saddr6->sin6_addr.s6_addr, &addr->ip6.addr, 16);
			saddr6->sin6_family = AF_INET6;
			saddr6->sin6_port = port;
			return 0;
		case SOCKSIZE:
			saddr4->sin_addr.s_addr = addr->ip4.addr;
			saddr4->sin_family = AF_INET;
			saddr4->sin_port = port;
			return 0;
		default:
			sc_errno = EINVAL;
			return -1;
	}
}
