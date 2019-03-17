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
#include <net/conv.h>
#include <pico_stack.h>
#include <pico_socket.h>
#include <pico_dhcp_client.h>
#include <tasks/task.h>
#include <fs/vfs.h>
#include <errno.h>
#include <endian.h>
#include <spinlock.h>

#ifdef ENABLE_PICOTCP

struct socket {
	struct pico_socket* pico_socket;
	int conn_requests;
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

static inline struct socket* get_socket(task_t* task, int sockfd) {
	vfs_file_t* fp = vfs_get_from_id(sockfd, task);
	if(!fp) {
		sc_errno = EBADF;
		return NULL;
	}

	if(fp->type != FT_IFSOCK) {
		sc_errno = ENOTSOCK;
		return NULL;
	}

	return (struct socket*)fp->mount_instance;
}

static void socket_cb(uint16_t ev, struct pico_socket* pico_sock) {
	struct socket* sock = (struct socket*)pico_sock->priv;
	if(!sock) {
		return;
	}

	if(ev & PICO_SOCK_EV_CONN) {
		if(sock->state == SOCK_LISTEN) {
			sock->conn_requests++;
		} else {
			sock->state = SOCK_CONNECTED;
		}
	}

	if(ev & PICO_SOCK_EV_ERR) {
		sock->state = SOCK_RESET_BY_PEER;
	}

	if(ev & PICO_SOCK_EV_FIN || ev & PICO_SOCK_EV_CLOSE) {
		sock->state = SOCK_CLOSED;
	}

	sock->can_write = (ev & PICO_SOCK_EV_WR);

	if((ev & PICO_SOCK_EV_RD) && spinlock_get(&net_pico_lock, 200)) {
		sock->read_buffer_length += pico_socket_read(sock->pico_socket,
			(void*)sock->read_buffer + sock->read_buffer_length, 0x5000);

		spinlock_release(&net_pico_lock);
		sock->can_write = true;
	}
}

static size_t vfs_read_cb(vfs_file_t* fp, void* dest, size_t size) {
	struct socket* sock = (struct socket*)(fp->mount_instance);

	if(sock->state == SOCK_CLOSED) {
		sc_errno = ENOTCONN;
		return -1;
	}
	if(sock->state == SOCK_RESET_BY_PEER) {
		sc_errno = ECONNRESET;
		return -1;
	}

	if(!sock->read_buffer_length && fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

	while(!sock->read_buffer_length) {
		if(sock->state == SOCK_CLOSED) {
			sc_errno = ENOTCONN;
			return -1;
		}
		if(sock->state == SOCK_RESET_BY_PEER) {
			sc_errno = ECONNRESET;
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
	return size;
}

static size_t vfs_write_cb(vfs_file_t* fp, void* source, size_t size) {
	struct socket* sock = (struct socket*)(fp->mount_instance);

	while(!sock->can_write) {
		if(sock->state == SOCK_CLOSED) {
			sc_errno = ENOTCONN;
			return -1;
		}
		if(sock->state == SOCK_RESET_BY_PEER) {
			sc_errno = ECONNRESET;
			return -1;
		}

		asm("hlt\n");
	}

	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}
	size_t written = pico_socket_write(sock->pico_socket, source, size);
	spinlock_release(&net_pico_lock);

	sc_errno = pico_err;
	return written;
}


int net_vfs_close_cb(vfs_file_t* fp) {
	struct socket* sock = (struct socket*)(fp->mount_instance);

	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}

	if(pico_socket_close(sock->pico_socket) < 0) {
		spinlock_release(&net_pico_lock);
		sc_errno = pico_err;
		return -1;
	}

	spinlock_release(&net_pico_lock);
	sock->state = SOCK_CLOSED;
	return 0;
}

vfs_file_t* new_socket_fd(task_t* task, struct pico_socket* pico_sock) {
	struct socket* sock = (struct socket*)zmalloc(sizeof(struct socket));
	sock->pico_socket = pico_sock;
	pico_sock->priv = (void*)sock;

	vfs_file_t* fd = vfs_alloc_fileno(task);
	fd->type = FT_IFSOCK;
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
	struct socket* sock = get_socket(task, sockfd);
	if(!sock) {
		return -1;
	}

	union pico_address pico_addr = { .ip4 = { 0 } };
	uint16_t port = net_bsd_to_pico_port(addr, addrlen);
	if(net_bsd_to_pico_addr(&pico_addr, addr, addrlen) < 0) {
		sc_errno = EINVAL;
		return -1;
	}

	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}

	if(pico_socket_bind(sock->pico_socket, &pico_addr, &port) < 0) {
		spinlock_release(&net_pico_lock);
		sc_errno = pico_err;
		return -1;
	}

	spinlock_release(&net_pico_lock);
	sock->state = SOCK_BOUND;
	return 0;
}

int net_listen(task_t* task, int sockfd, int backlog) {
	struct socket* sock = get_socket(task, sockfd);
	if(!sock) {
		return -1;
	}

	if(backlog < 4) {
		backlog = 4;
	}

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
	return 0;
}

int net_select(task_t* task, int nfds, fd_set *readfds, fd_set *writefds) {
	int events = 0;
	FD_ZERO(readfds);
	FD_ZERO(writefds);

	while(!events) {
		for(int num = 0; num < VFS_MAX_OPENFILES; num++) {
			if(!task->files[num].inode || task->files[num].type != FT_IFSOCK) {
				continue;
			}

			struct socket* sock = (struct socket*)task->files[num].mount_instance;
			if(sock->conn_requests || sock->read_buffer_length) {
				FD_SET(num, readfds);
				events++;
			}

			if(sock->can_write) {
				FD_SET(num, writefds);
				events++;
			}
		}
		asm("hlt");
	}

	return events;
}

int net_accept(task_t* task, int sockfd, struct sockaddr *addr,
	socklen_t *addrlen) {

	struct socket* sock = get_socket(task, sockfd);
	if(!sock) {
		return -1;
	}

	if(sock->state == SOCK_CONNECTED) {
		sc_errno = EBADF;
		return -1;
	}

	while(!sock->conn_requests) {
		asm("hlt\n");
	}

	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}

	union pico_address pico_addr;
	uint16_t port;
	struct pico_socket* pico_sock = pico_socket_accept(sock->pico_socket, &pico_addr, &port);
	spinlock_release(&net_pico_lock);
	net_conv_pico2bsd(addr, SOCKSIZE, &pico_addr, port);

	// FIXME
	int yes = 1;
	pico_socket_setoption(pico_sock, PICO_TCP_NODELAY, &yes);

	if(!pico_sock) {
		sc_errno = pico_err;
		return -1;
	}

	vfs_file_t* new_fd = new_socket_fd(task, pico_sock);
	sock->conn_requests--;
	return new_fd->num;
}

int net_getpeername(task_t* task, int sockfd, struct sockaddr* addr,
	socklen_t* addrlen) {

	struct socket* sock = get_socket(task, sockfd);
	if(!sock) {
		return -1;
	}

	return net_conv_pico2bsd(addr, *addrlen, &sock->pico_socket->remote_addr,
		sock->pico_socket->remote_port);
}

int net_getsockname(task_t* task, int sockfd, struct sockaddr* addr,
	socklen_t* addrlen) {

	struct socket* sock = get_socket(task, sockfd);
	if(!sock) {
		return -1;
	}

	return net_conv_pico2bsd(addr, *addrlen, &sock->pico_socket->local_addr,
		sock->pico_socket->local_port);
}

#endif /* ENABLE_PICOTCP */
