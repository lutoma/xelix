#pragma once

/* Copyright © 2011 Lukas Martini
 *
 * This file is part of Xlibc.
 *
 * Xlibc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Xlibc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Xlibc. If not, see <http://www.gnu.org/licenses/>.
 */
 
#define E2BIG 0
#define EACCES 1
#define EADDRINUSE 2
#define EADDRNOTAVAIL 3
#define EAFNOSUPPORT 4
#define EAGAIN 5
#define EALREADY 6
#define EBADF 7
#define EBADMSG 8
#define EBUSY 9 
#define ECANCELED 10
#define ECHILD 11
#define ECONNABORTED 12
#define ECONNREFUSED 13
#define ECONNRESET 14
#define EDEADLK 15
#define EDESTADDRREQ 16
#define EDOM 17
#define EDQUOT 18
#define EEXIST 19
#define EFAULT 20
#define EFBIG 21
#define EHOSTUNREACH 22
#define EIDRM 23
#define EILSEQ 24
#define EINPROGRESS 24
#define EINTR 25
#define EINVAL 26
#define EIO 27
#define EISCONN 28
#define EISDIR 29
#define ELOOP 30
#define EMFILE 31
#define EMLINK 32
#define EMSGSIZE 33
#define EMULTIHOP 34
#define ENAMETOOLONG 35
#define ENETDOWN 36
#define ENETRESET 37
#define ENETUNREACH 38
#define ENFILE 39
#define ENOBUFS 40
#define ENODATA 41
#define ENODEV 42
#define ENOENT 43
#define ENOEXEC 44
#define ENOLCK 45
#define ENOLINK 46
#define ENOMEM 47
#define ENOMSG 48
#define ENOPROTOOPT 49
#define ENOSPC 50
#define ENOSR 51
#define ENOSTR 52
#define ENOSYS 53
#define ENOTCONN 54
#define ENOTDIR 55
#define ENOTEMPTY 56
#define ENOTSOCK 57
#define ENOTSUP 58
#define ENOTTY 59
#define ENXIO 60
#define EOPNOTSUPP 61
#define EOVERFLOW 62
#define EPERM 63 
#define EPIPE 64
#define EPROTO 65
#define EPROTONOSUPPORT 66
#define EPROTOTYPE 67
#define ERANGE 68
#define EROFS 69
#define ESPIPE 70
#define ESRCH 71
#define ESTALE 72
#define ETIME 73
#define ETIMEDOUT 74
#define ETXTBSY 75
#define EWOULDBLOCK 76
#define EXDEV 77

// Defined in crt0.asm
extern int errno;
