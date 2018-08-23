/* Copyright © 2015-2018 Lukas Martini
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

#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

#include <stdint.h>

#define AF_UNSPEC 0
#define AF_UNIX 1
#define AF_INET 2

#define SOCK_DGRAM 1
#define SOCK_STREAM 2

/* SOCK_RDM is only really a linux thing from what I can tell (not part of
 * posix), but busybox needs it.
 */
#define SOCK_RDM 3
#define SOCK_RAW 4

#define SOCK_SEQPACKET 5

#define SOL_SOCKET 0

#define SO_DEBUG 1
#define SO_BROADCAST 2
#define SO_KEEPALIVE 3
#define SO_LINGER 4
#define SO_OOBINLINE 5
#define SO_SNDBUF 6
#define SO_RCVBUF 7
#define SO_DONTROUTE 8
#define SO_RCVLOWAT 9
#define SO_RCVTIMEO 10
#define SO_SNDLOWAT 11
#define SO_SNDTIMEO 12
#define SO_REUSEADDR 13

#define AF_UNSPEC 0
#define AF_INET 1
#define AF_INET6 2
#define AF_UNIX 3

typedef uint32_t socklen_t;
typedef uint32_t sa_family_t;

struct sockaddr {
	sa_family_t sa_family;
	char sa_data[];
};

int getsockname(int socket, struct sockaddr *restrict address,
	socklen_t *restrict address_len);

int getpeername(int socket, struct sockaddr *restrict address,
	socklen_t *restrict address_len);

int getsockopt(int socket, int level, int option_name,
       void *restrict option_value, socklen_t *restrict option_len);

int setsockopt(int socket, int level, int option_name,
	const void *option_value, socklen_t option_len);
#endif /* _SYS_SOCKET_H */
