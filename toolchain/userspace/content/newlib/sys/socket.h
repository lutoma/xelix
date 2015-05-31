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

typedef uint32_t socklen_t;
typedef uint32_t sa_family_t;

struct sockaddr {
	sa_family_t sa_family;
	char sa_data[];
};

#endif /* _SYS_SOCKET_H */