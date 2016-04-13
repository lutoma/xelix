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

#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

#include <stdint.h>

#define AF_UNSPEC 0
#define AF_UNIX 1
#define AF_INET 2

#define SOCK_DGRAM 1
#define SOCK_STREAM 2
#define SOCK_SEQPACKET 3

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

typedef uint32_t socklen_t;
typedef uint32_t sa_family_t;

struct sockaddr {
	sa_family_t sa_family;
	char sa_data[];
};

#endif /* _SYS_SOCKET_H */