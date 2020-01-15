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

#ifndef _NETDB_H
#define _NETDB_H
#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <sys/socket.h>

#define IPPORT_RESERVED 1024

#define AI_PASSIVE 1
#define AI_CANONNAME 2
#define AI_NUMERICHOST 4
#define AI_NUMERICSERV 8
#define AI_V4MAPPED 16
#define AI_ALL 32
#define AI_ADDRCONFIG 64

#define NI_NOFQDN 1
#define NI_NUMERICHOST 2
#define NI_NAMEREQD 4
#define NI_NUMERICSERV 8
#define NI_NUMERICSCOPE 16
#define NI_DGRAM 32

#define NI_MAXHOST 1025
#define NI_MAXSERV 32

#define EAI_AGAIN 1
#define EAI_BADFLAGS 2
#define EAI_FAIL 3
#define EAI_FAMILY 4
#define EAI_MEMORY 5
#define EAI_NONAME 6
#define EAI_SERVICE 7
#define EAI_SOCKTYPE 8
#define EAI_SYSTEM 9
#define EAI_OVERFLOW 10

/* Possible values left in `h_errno'.  */
# define HOST_NOT_FOUND	1	/* Authoritative Answer Host not found.  */
# define TRY_AGAIN	2	/* Non-Authoritative Host not found,
				   or SERVERFAIL.  */
# define NO_RECOVERY	3	/* Non recoverable errors, FORMERR, REFUSED,
				   NOTIMP.  */
# define NO_DATA	4	/* Valid name, no data record of requested
				   type.  */
# define NETDB_INTERNAL	-1	/* See errno.  */
# define NETDB_SUCCESS	0	/* No problem.  */
# define NO_ADDRESS	NO_DATA	/* No address, look for MX record.  */

struct hostent {
	char *h_name;
	char **h_aliases;
	int h_addrtype;
	int h_length;
	char **h_addr_list;

	// Backward compatibility - same as h_addr_list[0]
	char *h_addr;
};

struct netent {
	char *n_name;
	char **n_aliases;
	int n_addrtype;
	uint32_t n_net;
};

struct protoent {
	char *p_name;
	char **p_aliases;
	int p_proto;
};

struct servent {
	char *s_name;
	char **s_aliases;
	int s_port;
	char *s_proto;
};

struct addrinfo {
	int ai_flags;
	int ai_family;
	int ai_socktype;
	int ai_protocol;
	socklen_t ai_addrlen;
	struct sockaddr *ai_addr;
	char *ai_canonname;
	struct addrinfo *ai_next;
};

extern int h_errno;

void endhostent(void);
void endnetent(void);
void endprotoent(void);
void endservent(void);
void freeaddrinfo(struct addrinfo *);
const char *gai_strerror(int);
const char *hstrerror(int err);

int getaddrinfo(const char *restrict, const char *restrict,
	const struct addrinfo *restrict, struct addrinfo **restrict);

struct hostent *gethostent(void);

int getnameinfo(const struct sockaddr *restrict, socklen_t,
	char *restrict, socklen_t, char *restrict, socklen_t, int);

struct netent *getnetbyaddr(uint32_t, int);
struct netent *getnetbyname(const char *);
struct netent *getnetent(void);
struct protoent *getprotobyname(const char *);
struct protoent *getprotobynumber(int);
struct protoent *getprotoent(void);
struct servent *getservbyname(const char *, const char *);
struct servent *getservbyport(int, const char *);
struct servent *getservent(void);
void sethostent(int);
void setnetent(int);
void setprotoent(int);
void setservent(int);

#ifdef __cplusplus
}       /* C++ */
#endif
#endif /* _NETDB_H */
