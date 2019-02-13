/* socket.c: Userland socket API
 * Copyright © 2019 Lukas Martini
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

#include "socket.h"
#include <net/net.h>
#include <pico_stack.h>
#include <pico_socket.h>
#include <pico_dhcp_client.h>
#include <tasks/task.h>
#include <fs/vfs.h>
#include <errno.h>
#include <endian.h>
#include <spinlock.h>

#define SOCKSIZE sizeof(struct sockaddr_in)
#define SOCKSIZE6 sizeof(struct sockaddr_in6)

struct socket {
	struct socket* next;
	struct pico_socket* pico_socket;
	int conn_requests;
	bool can_read;
	bool can_write;
	char read_buffer[0x5000];
	size_t read_buffer_length;

	enum {
		SOCK_OPEN,
		SOCK_BOUND,
		SOCK_LISTEN,
		SOCK_CONNECTED,
		SOCK_ERROR,
		SOCK_RESET_BY_PEER,
		SOCK_CLOSED,
	} state;
};


static int bsd_to_pico_addr(union pico_address *addr, const struct sockaddr *_saddr, socklen_t socklen) {
    if(socklen != SOCKSIZE && socklen != SOCKSIZE6) {
    	return -1;
    }

    if (socklen == SOCKSIZE6) {
        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)_saddr;
        memcpy(&addr->ip6.addr, &saddr->sin6_addr.s6_addr, 16);
        saddr->sin6_family = AF_INET6;
    } else {
        struct sockaddr_in *saddr = (struct sockaddr_in *)_saddr;
        addr->ip4.addr = saddr->sin_addr.s_addr;
        saddr->sin_family = AF_INET;
    }
    return 0;
}

static uint16_t bsd_to_pico_port(const struct sockaddr *_saddr, socklen_t socklen) {
    if(socklen != SOCKSIZE && socklen != SOCKSIZE6) {
    	return -1;
    }

    if (socklen == SOCKSIZE6) {
        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)_saddr;
        return saddr->sin6_port;
    } else {
        struct sockaddr_in *saddr = (struct sockaddr_in *)_saddr;
        return saddr->sin_port;
    }
}

static inline vfs_file_t* get_socket_fp(task_t* task, int sockfd) {
	vfs_file_t* fp = vfs_get_from_id(sockfd, task);
	if(!fp) {
		sc_errno = EBADF;
		return NULL;
	}

	if(fp->type != VFS_FILE_TYPE_SOCKET) {
		sc_errno = ENOTSOCK;
		return NULL;
	}

	return fp;
}

static void socket_cb(uint16_t ev, struct pico_socket* pico_sock) {
	struct socket* sock = (struct socket*)pico_sock->priv;

	if(!sock) {
		serial_printf("socket_cb: No matching socket found.\n");
		return;
	}

	if(ev & PICO_SOCK_EV_CONN) {
		serial_printf("New state SOCK_CONNECTED\n");
		if(sock->state == SOCK_LISTEN) {
			sock->conn_requests++;
		} else {
			sock->state = SOCK_CONNECTED;
		}
	}

	if(ev & PICO_SOCK_EV_ERR) {
		serial_printf("New state SOCK_RESET_BY_PEER\n");
		sock->state = SOCK_RESET_BY_PEER;
	}

	if(ev & PICO_SOCK_EV_FIN || ev & PICO_SOCK_EV_CLOSE) {
		serial_printf("New state SOCK_CLOSED\n");
		sock->state = SOCK_CLOSED;
	}

	sock->can_read = (ev & PICO_SOCK_EV_RD);
	sock->can_write = (ev & PICO_SOCK_EV_WR);


	if(sock->can_read && spinlock_get(&net_pico_lock, 200)) {
		sock->read_buffer_length += pico_socket_read(sock->pico_socket,
			(void*)sock->read_buffer + sock->read_buffer_length, 0x5000);

		spinlock_release(&net_pico_lock);
		sock->can_write = true;
	}
	serial_printf("socket cb, can_read: %d, can_write: %d\n", sock->can_read, sock->can_write);
}

static size_t vfs_read_cb(vfs_file_t* fp, void* dest, size_t size) {
	struct socket* sock = (struct socket*)(fp->mount_instance);

	if(!sock->read_buffer_length && fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

	while(!sock->read_buffer_length) {
		if(sock->state == SOCK_CLOSED) {
			return 0;
		}
		if(sock->state == SOCK_RESET_BY_PEER) {
			sc_errno = ENOTCONN;
			return -1;
		}

		asm("hlt");
	}

	if(size > sock->read_buffer_length) {
		size = sock->read_buffer_length;
	}
	memcpy(dest, sock->read_buffer, size);
	sock->read_buffer_length -= size;
	if(sock->read_buffer_length) {
		memmove(sock->read_buffer, sock->read_buffer + size, sock->read_buffer_length);
	}
	serial_printf("vfs_read_cb, data length was %d\n", size);
	return size;
/*
	size_t read = pico_socket_read(sock->pico_socket, dest, size);
	spinlock_release(&net_pico_lock);
	if(read < 0) {
		if(pico_err == PICO_ERR_ESHUTDOWN) {
			return 0;
		}

		sc_errno = pico_err;
		return -1;
	}

	return read;
*/
}

static size_t vfs_write_cb(vfs_file_t* fp, void* source, size_t size) {
	serial_printf("net_vfs_write sock %d len %d data \"%s\"\n", fp->num, size, strndup(source, size));
	struct socket* sock = (struct socket*)(fp->mount_instance);

	while(!sock->can_write) {
		if(sock->state == SOCK_CLOSED) {
			return 0;
		}
		if(sock->state == SOCK_RESET_BY_PEER) {
			sc_errno = ENOTCONN;
			return -1;
		}

		asm("hlt\n");
	}

	serial_printf("can_write, sending now.\n");
	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}
	size_t written = pico_socket_write(sock->pico_socket, source, size);
	spinlock_release(&net_pico_lock);

	if(written < 0) {
		sc_errno = pico_err;
		return -1;
	}
	return written;
}

vfs_file_t* new_socket_fd(task_t* task, struct pico_socket* pico_sock) {
	struct socket* sock = (struct socket*)zmalloc(sizeof(struct socket));
	sock->pico_socket = pico_sock;
	pico_sock->priv = (void*)sock;

	vfs_file_t* fd = vfs_alloc_fileno(task);
	fd->type = VFS_FILE_TYPE_SOCKET;
	fd->flags = O_RDWR;
	fd->callbacks.read = vfs_read_cb;
	fd->callbacks.write = vfs_write_cb;
	fd->mount_instance = (void*)sock;
	return fd;
}

int net_socket(task_t* task, int domain, int type, int protocol) {
	switch(domain) {
		case AF_INET: domain = PICO_PROTO_IPV4; break;
		case AF_INET6: domain = PICO_PROTO_IPV6; break;
		default:
			sc_errno = EAFNOSUPPORT;
			return -1;
	}

	switch(type) {
		case SOCK_STREAM: type = PICO_PROTO_TCP; break;
		case SOCK_DGRAM: type = PICO_PROTO_UDP; break;
		default:
			sc_errno = EPROTONOSUPPORT;
			return -1;
	}

	serial_printf("net_socket domain %d type %d protocol %d\n", domain, type, protocol);

	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}

	struct pico_socket* pico_sock = pico_socket_open(domain, type, &socket_cb);
	if(!pico_sock) {
		sc_errno = pico_err;
		return -1;
	}

	spinlock_release(&net_pico_lock);
	vfs_file_t* fd = new_socket_fd(task, pico_sock);
	return fd->num;
}

int net_bind(task_t* task, int sockfd, const struct sockaddr* addr,
	socklen_t addrlen) {
	vfs_file_t* fd = get_socket_fp(task, sockfd);
	if(!fd) {
		return -1;
	}

	union pico_address pico_addr = { .ip4 = { 0 } };
/*	if(bsd_to_pico_addr(&pico_addr, addr, addrlen) < 0) {
		sc_errno = EINVAL;
		return -1;
	}
*/
	uint16_t port = bsd_to_pico_port(addr, addrlen);


	char ipbuf[15];
	pico_ipv4_to_string(ipbuf, pico_addr.ip4.addr);
	serial_printf("net_bind sockfd %d %s port %d\n", sockfd, ipbuf, endian_swap16(port));

	struct socket* sock = (struct socket*)fd->mount_instance;
	serial_printf("pico_socket at 0x%x, proto at 0x%x\n", fd->mount_instance, sock->pico_socket->proto);

	struct pico_ip4 inaddr_any = {0};
	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}

	if(pico_socket_bind(sock->pico_socket, &inaddr_any, &port) < 0) {
		spinlock_release(&net_pico_lock);
		sc_errno = pico_err;
		return -1;
	}

	spinlock_release(&net_pico_lock);
	sock->state = SOCK_BOUND;
	return 0;
}

int net_listen(task_t* task, int sockfd, int backlog) {
	vfs_file_t* fd = get_socket_fp(task, sockfd);
	if(!fd) {
		return -1;
	}

	// FIXME Check proper handling for this
	if(backlog == -1) {
		backlog = 40;
	}

	struct socket* sock = (struct socket*)fd->mount_instance;

	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}

	if(pico_socket_listen(sock->pico_socket, backlog) < 0) {
		spinlock_release(&net_pico_lock);
		sc_errno = pico_err;
		return -1;
	}

	spinlock_release(&net_pico_lock);
	sock->state = SOCK_LISTEN;
	serial_printf("net_listen %d backlog %d\n", sockfd, backlog);
	return 0;
}

int net_select(task_t* task, int nfds, fd_set *readfds, fd_set *writefds) {
	serial_printf("net_select nfds %d\n", nfds);
	for(int num = 0;; num = ++num % VFS_MAX_OPENFILES) {
		if(!task->files[num].inode || task->files[num].type != VFS_FILE_TYPE_SOCKET) {
			continue;
		}

		struct socket* sock = (struct socket*)task->files[num].mount_instance;
		if(sock->conn_requests) {
			serial_printf("something happened.\n");

			// FIXME set readfds/writefds
			return 1;
		}

		asm("hlt");
	}

	return -1;
}

int net_accept(task_t* task, int sockfd, struct sockaddr *addr,
	socklen_t *addrlen) {

	vfs_file_t* fd = get_socket_fp(task, sockfd);
	if(!fd) {
		return -1;
	}


	struct socket* sock = (struct socket*)fd->mount_instance;
	if(sock->state == SOCK_CONNECTED) {
		sc_errno = EBADF;
		return -1;
	}

	while(!sock->conn_requests) {
		asm("hlt\n");
	}

	union pico_address pico_addr;
	uint16_t port;

	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}
	struct pico_socket* pico_sock = pico_socket_accept(sock->pico_socket, &pico_addr, &port);
	int yes = 1;
	pico_socket_setoption(pico_sock, PICO_TCP_NODELAY, &yes);

	spinlock_release(&net_pico_lock);
	if(!pico_sock) {
		serial_printf("net_accept pico error.\n");
		sc_errno = pico_err;
		return -1;
	}

	vfs_file_t* new_fd = new_socket_fd(task, pico_sock);
	sock->conn_requests--;
	serial_printf("net_accept %d, new %d\n", sockfd, new_fd->num);
	return new_fd->num;
}
