#pragma once

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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <tasks/task.h>

#define SOCK_DGRAM 1
#define SOCK_STREAM 2
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

#define MSG_CTRUNC 1
#define MSG_DONTROUTE 2
#define MSG_EOR 4
#define MSG_OOB 8
#define MSG_NOSIGNAL 16
#define MSG_PEEK 32
#define MSG_TRUNC 64
#define MSG_WAITALL 128

#define AF_UNSPEC 0
#define AF_INET 1
#define AF_INET6 2
#define AF_UNIX 3

#define IPPROTO_IP 0x0
#define IPPROTO_ICMP 0x1
#define IPPROTO_TCP 0x06
#define IPPROTO_UDP 0x11

#define INADDR_ANY 1
#define INADDR_BROADCAST 2
#define INADDR_NONE 3

#define PF_INET AF_INET

typedef uint32_t socklen_t;
typedef uint32_t sa_family_t;
typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct sockaddr {
	sa_family_t sa_family;
	char sa_data[];
};

struct in_addr {
	in_addr_t s_addr;
};

struct in6_addr {
	uint8_t s6_addr[16];
};

struct sockaddr_in {
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
	char sin_zero[8];
};

struct sockaddr_in6 {
	sa_family_t sin6_family;
	in_port_t sin6_port;
	uint32_t sin6_flowinfo;
	struct in6_addr sin6_addr;
	uint32_t sin6_scope_id;
};


/* fd_set handling from newlib */
typedef	unsigned long	fd_mask;

#define	FD_SETSIZE	64
#define	NFDBITS	(sizeof (fd_mask) * 8)	/* bits per mask */
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1L << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1L << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1L << ((n) % NFDBITS)))
#define	FD_ZERO(p)	(__extension__ (void)({ \
     size_t __i; \
     char *__tmp = (char *)p; \
     for (__i = 0; __i < sizeof (*(p)); ++__i) \
       *__tmp++ = 0; \
}))
#define	_howmany(x,y)	(((x)+((y)-1))/(y))

typedef	struct {
	fd_mask	fds_bits[_howmany(FD_SETSIZE, NFDBITS)];
} fd_set;


#define SOCKSIZE sizeof(struct sockaddr_in)
#define SOCKSIZE6 sizeof(struct sockaddr_in6)

struct recvfrom_data {
	int sockfd;
	void* dest;
	size_t size;
	int flags;
	struct sockaddr* src_addr;
	socklen_t *addrlen;
};

int net_vfs_close_cb(vfs_file_t* fp);
int net_recvfrom(task_t* task, struct recvfrom_data* data, int struct_size);
int net_socket(task_t* task, int domain, int type, int protocol);
int net_bind(task_t* task, int sockfd, const struct sockaddr* addr,
	socklen_t addrlen);
int net_listen(task_t* task, int sockfd, int backlog);
int net_accept(task_t* task, int sockfd, struct sockaddr *addr,
	socklen_t *addrlen);
int net_getpeername(task_t* task, int sockfd, struct sockaddr* addr,
	socklen_t* addrlen);
int net_getsockname(task_t* task, int sockfd, struct sockaddr* addr,
	socklen_t* addrlen);
int net_getaddr(task_t* task, const char* host, char* result, int result_len);
int net_getname(task_t* task, const char* ip, char* result, int result_len);
int net_connect(task_t* task, int socket, const struct sockaddr* address, uint32_t address_len);
