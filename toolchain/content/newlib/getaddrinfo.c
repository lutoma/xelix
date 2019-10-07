/* Copyright © 2019 Lukas Martini
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/xelix.h>


int getaddrinfo(const char *node, const char *service,
                    const struct addrinfo *hints,
                    struct addrinfo **res) {

	size_t len = strlen(node);
	if(len < 1) {
		errno = EINVAL;
		return -1;
	}

	struct in_addr in;
	// Could already be an IP
	if(!inet_aton(node, &in)) {
		// Resolve
		char* result = malloc(256);
		if(syscall(46, (uintptr_t)node, (uintptr_t)result, 256) != 0) {
			return -1;
		}

		if(!inet_aton(result, &in)) {
			free(result);
			return -1;
		}

		free(result);
	}

	struct sockaddr_in* sin = calloc(1, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;
	sin->sin_port = 0;
	sin->sin_addr.s_addr = in.s_addr;

	struct addrinfo* ai = calloc(1, sizeof(struct addrinfo));
	ai->ai_family = AF_INET;
	ai->ai_socktype = SOCK_STREAM;
	ai->ai_protocol = IPPROTO_TCP;
	ai->ai_addrlen = sizeof(struct sockaddr_in);
	ai->ai_addr = (struct sockaddr*)sin;

	if(res) {
		*res = ai;
	}
	return 0;
}

int getnameinfo(const struct sockaddr* addr, socklen_t addrlen,
                       char* host, socklen_t hostlen,
                       char* serv, socklen_t servlen, int flags) {

	if(addrlen < sizeof(struct sockaddr_in)) {
		return -1;
	}

	const struct sockaddr_in* in = (const struct sockaddr_in*)addr;
	unsigned char* bytes = (unsigned char *)&in->sin_addr.s_addr;

	if(host && hostlen) {
		if(flags & NI_NUMERICHOST) {
			snprintf(host, hostlen, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
		} else {
			char ipb[18];
			snprintf(ipb, 18, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);

			if(syscall(47, (uintptr_t)ipb, (uintptr_t)host, hostlen) != 0) {
				if(flags & NI_NAMEREQD) {
					errno = EAI_NONAME;
					return -1;
				} else {
					strncpy(host, ipb, hostlen);
				}
			}
		}
	}

	if(serv && servlen) {
		snprintf(serv, servlen, "%d", in->sin_port);
	}
	return 0;
}
