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
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
	if(argc != 2) {
		fprintf(stderr, "Usage: %s <host>\n", argv[0]);
		return -1;
	}

	struct in_addr in;
	struct addrinfo* addr = NULL;
	char host[512];

	// Do reverse lookup if IP
	if(inet_aton(argv[1], &in) != 0) {
		const struct sockaddr_in sin = {
			.sin_family = AF_INET,
			.sin_port = 0,
			.sin_addr = in,
		};

		if(getnameinfo((struct sockaddr*)&sin, sizeof(sin), host, 512, NULL, 0, NI_NAMEREQD) == 0) {
			printf("%s has address %s\n", argv[1], host);
			return 0;
		}
	} else {
		if(getaddrinfo(argv[1], NULL, NULL, &addr) == 0) {
			getnameinfo(addr->ai_addr, addr->ai_addrlen, host, 512, NULL, 0, NI_NUMERICHOST);
			printf("%s has address %s\n", argv[1], host);
			return 0;
		}
	}

	printf("Host %s not found.\n", argv[1]);
	return -1;
}

