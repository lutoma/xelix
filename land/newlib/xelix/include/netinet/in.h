/* Copyright © 2015-2019 Lukas Martini
 * Copyright © 1991-2018 Free Software Foundation, Inc.
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
#include <sys/socket.h>

#define IPPROTO_IP 0x0
#define IPPROTO_ICMP 0x1
#define IPPROTO_TCP 0x06
#define IPPROTO_UDP 0x11

#define INADDR_ANY 0
#define INADDR_NONE -1
#define INADDR_BROADCAST 0xFFFFFFFF
#define INADDR_LOOPBACK 0x7f000001

#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46

/* These macros are all nonstandard (?), but required by wget (and probably
 * other GNU stuff). Taken from glibc netinet/in.h.
 */
#ifdef __GNUC__
# define IN6_IS_ADDR_UNSPECIFIED(a) \
  (__extension__                                                              \
   ({ const struct in6_addr *__a = (const struct in6_addr *) (a);              \
      __a->__in6_u.__u6_addr32[0] == 0                                              \
      && __a->__in6_u.__u6_addr32[1] == 0                                      \
      && __a->__in6_u.__u6_addr32[2] == 0                                      \
      && __a->__in6_u.__u6_addr32[3] == 0; }))
# define IN6_IS_ADDR_LOOPBACK(a) \
  (__extension__                                                              \
   ({ const struct in6_addr *__a = (const struct in6_addr *) (a);              \
      __a->__in6_u.__u6_addr32[0] == 0                                              \
      && __a->__in6_u.__u6_addr32[1] == 0                                      \
      && __a->__in6_u.__u6_addr32[2] == 0                                      \
      && __a->__in6_u.__u6_addr32[3] == htonl (1); }))
# define IN6_IS_ADDR_LINKLOCAL(a) \
  (__extension__                                                              \
   ({ const struct in6_addr *__a = (const struct in6_addr *) (a);              \
      (__a->__in6_u.__u6_addr32[0] & htonl (0xffc00000)) == htonl (0xfe800000); }))
# define IN6_IS_ADDR_SITELOCAL(a) \
  (__extension__                                                              \
   ({ const struct in6_addr *__a = (const struct in6_addr *) (a);              \
      (__a->__in6_u.__u6_addr32[0] & htonl (0xffc00000)) == htonl (0xfec00000); }))
# define IN6_IS_ADDR_V4MAPPED(a) \
  (__extension__                                                              \
   ({ const struct in6_addr *__a = (const struct in6_addr *) (a);              \
      __a->__in6_u.__u6_addr32[0] == 0                                              \
      && __a->__in6_u.__u6_addr32[1] == 0                                      \
      && __a->__in6_u.__u6_addr32[2] == htonl (0xffff); }))
# define IN6_IS_ADDR_V4COMPAT(a) \
  (__extension__                                                              \
   ({ const struct in6_addr *__a = (const struct in6_addr *) (a);              \
      __a->__in6_u.__u6_addr32[0] == 0                                              \
      && __a->__in6_u.__u6_addr32[1] == 0                                      \
      && __a->__in6_u.__u6_addr32[2] == 0                                      \
      && ntohl (__a->__in6_u.__u6_addr32[3]) > 1; }))
# define IN6_ARE_ADDR_EQUAL(a,b) \
  (__extension__                                                              \
   ({ const struct in6_addr *__a = (const struct in6_addr *) (a);              \
      const struct in6_addr *__b = (const struct in6_addr *) (b);              \
      __a->__in6_u.__u6_addr32[0] == __b->__in6_u.__u6_addr32[0]              \
      && __a->__in6_u.__u6_addr32[1] == __b->__in6_u.__u6_addr32[1]              \
      && __a->__in6_u.__u6_addr32[2] == __b->__in6_u.__u6_addr32[2]              \
      && __a->__in6_u.__u6_addr32[3] == __b->__in6_u.__u6_addr32[3]; }))
#else
# define IN6_IS_ADDR_UNSPECIFIED(a) \
        (((const uint32_t *) (a))[0] == 0                                      \
         && ((const uint32_t *) (a))[1] == 0                                      \
         && ((const uint32_t *) (a))[2] == 0                                      \
         && ((const uint32_t *) (a))[3] == 0)
# define IN6_IS_ADDR_LOOPBACK(a) \
        (((const uint32_t *) (a))[0] == 0                                      \
         && ((const uint32_t *) (a))[1] == 0                                      \
         && ((const uint32_t *) (a))[2] == 0                                      \
         && ((const uint32_t *) (a))[3] == htonl (1))
# define IN6_IS_ADDR_LINKLOCAL(a) \
        ((((const uint32_t *) (a))[0] & htonl (0xffc00000))                      \
         == htonl (0xfe800000))
# define IN6_IS_ADDR_SITELOCAL(a) \
        ((((const uint32_t *) (a))[0] & htonl (0xffc00000))                      \
         == htonl (0xfec00000))
# define IN6_IS_ADDR_V4MAPPED(a) \
        ((((const uint32_t *) (a))[0] == 0)                                      \
         && (((const uint32_t *) (a))[1] == 0)                                      \
         && (((const uint32_t *) (a))[2] == htonl (0xffff)))
# define IN6_IS_ADDR_V4COMPAT(a) \
        ((((const uint32_t *) (a))[0] == 0)                                      \
         && (((const uint32_t *) (a))[1] == 0)                                      \
         && (((const uint32_t *) (a))[2] == 0)                                      \
         && (ntohl (((const uint32_t *) (a))[3]) > 1))
# define IN6_ARE_ADDR_EQUAL(a,b) \
        ((((const uint32_t *) (a))[0] == ((const uint32_t *) (b))[0])              \
         && (((const uint32_t *) (a))[1] == ((const uint32_t *) (b))[1])      \
         && (((const uint32_t *) (a))[2] == ((const uint32_t *) (b))[2])      \
         && (((const uint32_t *) (a))[3] == ((const uint32_t *) (b))[3]))
#endif
#define IN6_IS_ADDR_MULTICAST(a) (((const uint8_t *) (a))[0] == 0xff)


typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr {
	in_addr_t s_addr;
};

struct in6_addr {
	union {
		uint8_t __u6_addr8[16];
		uint16_t __u6_addr16[8];
		uint32_t __u6_addr32[4];
	} __in6_u;
	#define s6_addr __in6_u.__u6_addr8
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

// Include at the end as it needs some of our typedefs
#include <arpa/inet.h>

#endif /* _NETINET_IN_H */
