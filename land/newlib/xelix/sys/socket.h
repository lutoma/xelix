/* Copyright © 2015-2019 Lukas Martini
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
#include <sys/uio.h>

#define SOCK_DGRAM 1
#define SOCK_STREAM 2

/* SOCK_RDM is only really a linux thing from what I can tell (not part of
 * posix), but busybox needs it.
 */
#define SOCK_RDM 3
#define SOCK_RAW 4

#define SOCK_SEQPACKET 5

#define SOL_SOCKET 1

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
#define SO_ACCEPTCONN 14
#define SO_ERROR 15
#define SO_TYPE 16

#define AF_UNSPEC 0
#define AF_INET 1
#define AF_INET6 2
#define AF_UNIX 3
#define PF_UNSPEC AF_UNSPEC
#define PF_INET AF_INET
#define PF_INET6 AF_INET6
#define PF_UNIX AF_UNIX

#define MSG_CTRUNC 1
#define MSG_DONTROUTE 2
#define MSG_EOR 4
#define MSG_OOB 8
#define MSG_NOSIGNAL 16
#define MSG_PEEK 32
#define MSG_TRUNC 64
#define MSG_WAITALL 128

#define SHUT_RD 1
#define SHUT_RDWR 2
#define SHUT_WR 3

#define SOMAXCONN 128
#define SCM_RIGHTS -1

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t socklen_t;
typedef uint32_t sa_family_t;

struct sockaddr {
	sa_family_t sa_family;
	char sa_data[];
};

struct msghdr {
	void *msg_name;
	socklen_t msg_namelen;
	struct iovec *msg_iov;
	int msg_iovlen;
	void *msg_control;
	socklen_t msg_controllen;
	int msg_flags;
};

struct cmsghdr {
	socklen_t cmsg_len;
	int cmsg_level;
	int cmsg_type;
};

struct linger {
	int l_onoff;
	int l_linger;
};

int accept(int, struct sockaddr *restrict, socklen_t *restrict);
int bind(int, const struct sockaddr *, socklen_t);
int connect(int, const struct sockaddr *, socklen_t);
int getpeername(int, struct sockaddr *restrict, socklen_t *restrict);
int getsockname(int, struct sockaddr *restrict, socklen_t *restrict);
int getsockopt(int, int, int, void *restrict, socklen_t *restrict);
int listen(int, int);
ssize_t recv(int, void *, size_t, int);
ssize_t recvfrom(int, void *restrict, size_t, int, struct sockaddr *restrict, socklen_t *restrict);
ssize_t recvmsg(int, struct msghdr *, int);
ssize_t send(int, const void *, size_t, int);
ssize_t sendmsg(int, const struct msghdr *, int);
ssize_t sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
int setsockopt(int, int, int, const void *, socklen_t);
int shutdown(int, int);
int sockatmark(int);
int socket(int, int, int);
int socketpair(int, int, int, int [2]);

#ifdef __cplusplus
}       /* C++ */
#endif
#endif /* _SYS_SOCKET_H */
