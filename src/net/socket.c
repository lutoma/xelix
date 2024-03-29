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
#include <pico_dns_client.h>
#include <tasks/task.h>
#include <tasks/syscall.h>
#include <fs/vfs.h>
#include <fs/poll.h>
#include <errno.h>
#include <endian.h>
#include <spinlock.h>

#ifdef CONFIG_ENABLE_PICOTCP
#define READ_BUFFER_SIZE 0x5000

#ifdef CONFIG_SOCKET_DEBUG
 #define debug(args...) log(LOG_DEBUG, "socket: " args)
#else
 #define debug(args...)
#endif

struct socket {
	struct pico_socket* pico_socket;
	int conn_requests;
	bool can_write;
	char read_buffer[READ_BUFFER_SIZE];
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
	if(unlikely(!fp)) {
		sc_errno = EBADF;
		return NULL;
	}

	if(unlikely(fp->type != FT_IFSOCK)) {
		sc_errno = ENOTSOCK;
		return NULL;
	}

	return (struct socket*)fp->mount_instance;
}

static void socket_cb(uint16_t ev, struct pico_socket* pico_sock) {
	int_disable();

	struct socket* sock = (struct socket*)pico_sock->priv;
	if(unlikely(!sock)) {
		//log(LOG_WARN, "net: socket_cb called without socket\n");
		return;
	}

	if(ev & PICO_SOCK_EV_CONN) {
		if(sock->state == SOCK_LISTEN) {
			debug("New client connection\n");
			sock->conn_requests++;
		} else {
			debug("Connection established\n");
			sock->state = SOCK_CONNECTED;
		}
	}

	if(ev & PICO_SOCK_EV_ERR) {
		debug("PICO_SOCK_EV_ERR, connection reset\n");
		sock->state = SOCK_RESET_BY_PEER;
	}

	if(ev & PICO_SOCK_EV_FIN || ev & PICO_SOCK_EV_CLOSE) {
		debug("Connection closed\n");
		sock->state = SOCK_CLOSED;
	}

	sock->can_write = (ev & PICO_SOCK_EV_WR);

	if((ev & PICO_SOCK_EV_RD) && spinlock_get(&net_pico_lock, 200)) {
			debug("cb sock %#x pico %#x\n", pico_sock->priv, pico_sock);

		debug("Socket read\n");
		int free_bytes = READ_BUFFER_SIZE - sock->read_buffer_length;

		if(free_bytes) {
			sock->read_buffer_length += pico_socket_read(sock->pico_socket,
				(void*)sock->read_buffer + sock->read_buffer_length, free_bytes);
		} else {
			log(LOG_WARN, "net: Socket %#x read buffer exhausted\n", sock);
		}

		spinlock_release(&net_pico_lock);
		sock->can_write = true;
		debug("Read done, buffer size %#x\n", sock->read_buffer_length);
	}
	int_enable();
}

// Only does recv() functionality for now
static size_t do_recvfrom(struct socket* sock, void* dest, size_t size,
	int fp_flags, int recv_flags, struct sockaddr* src_addr,
	socklen_t* addrlen) {

	if(sock->state == SOCK_CLOSED) {
		sc_errno = ENOTCONN;
		return -1;
	}
	if(sock->state == SOCK_RESET_BY_PEER) {
		sc_errno = ECONNRESET;
		return -1;
	}

	if(!sock->read_buffer_length && fp_flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

	int_enable();
	while(!sock->read_buffer_length) {
		if(sock->state == SOCK_CLOSED) {
			sc_errno = ENOTCONN;
			return -1;
		}
		if(sock->state == SOCK_RESET_BY_PEER) {
			sc_errno = ECONNRESET;
			return -1;
		}

		scheduler_yield();
	}
	int_disable();

	if(size > sock->read_buffer_length) {
		size = sock->read_buffer_length;
	}
	memcpy(dest, sock->read_buffer, size);

	if(!(recv_flags & MSG_PEEK)) {
		sock->read_buffer_length -= size;
		if(sock->read_buffer_length) {
			memmove(sock->read_buffer, sock->read_buffer + size, sock->read_buffer_length);
		}
	}

	return size;
}

static size_t vfs_read_cb(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	struct socket* sock = (struct socket*)(ctx->fp->mount_instance);
	if(!sock) {
		sc_errno = EBADF;
		return -1;
	}

	return do_recvfrom(sock, dest, size, ctx->fp->flags, 0, NULL, NULL);
}

int net_recvfrom(task_t* task, struct recvfrom_data* data, int struct_size) {
	struct socket* sock = get_socket(task, data->sockfd);
	if(!sock) {
		return -1;
	}

	vm_alloc_t alloc;
	void* dest = vm_map(VM_KERNEL, &alloc, &task->vmem, data->dest,
		data->size, VM_MAP_USER_ONLY | VM_RW);

	if(!dest) {
		task_signal(task, NULL, SIGSEGV);
		sc_errno = EFAULT;
		return -1;
	}

	size_t read = do_recvfrom(sock, dest, data->size,
		0, data->flags, data->src_addr, data->addrlen);

	// FIXME
	//vm_free(&alloc);
	return read;
}

static size_t vfs_write_cb(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	struct socket* sock = (struct socket*)(ctx->fp->mount_instance);

	int_enable();
	while(!sock->can_write) {
		if(sock->state == SOCK_CLOSED) {
			sc_errno = ENOTCONN;
			return -1;
		}
		if(sock->state == SOCK_RESET_BY_PEER) {
			sc_errno = ECONNRESET;
			return -1;
		}

		scheduler_yield();
	}
	int_disable();

	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}
	size_t written = pico_socket_write(sock->pico_socket, source, size);
	spinlock_release(&net_pico_lock);

	sc_errno = pico_err;
	return written;
}

void* lp = 0;

static int vfs_poll_cb(struct vfs_callback_ctx* ctx, int events) {
	struct socket* sock = (struct socket*)(ctx->fp->mount_instance);
	int ret = 0;

	//if(lp != sock) {
	//	lp = sock;
	//	debug("polling %#x pico %#x\n", sock, sock->pico_socket);
	//}

	//debug("poll %#x pico %#x\n", sock, sock->pico_socket);
	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}

	if(events & POLLIN && (sock->conn_requests || sock->read_buffer_length)) {
		debug("POLLIN %#x\n", sock->read_buffer_length);
		ret |= POLLIN;
	}
	if(events & POLLOUT && sock->can_write) {
		debug("POLLOUT\n");
		ret |= POLLOUT;
	}

	spinlock_release(&net_pico_lock);
	return ret;
}

int net_vfs_close_cb(vfs_file_t* fp) {
	// Not as simple as that – may still be open in fork. Need to do
	// refcounting with VFS layer tools
/*	struct socket* sock = (struct socket*)(fp->mount_instance);

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

	// FIXME Should this actually be set here or via the callback afterwards
	sock->state = SOCK_CLOSED;
*/
	return 0;
}

static vfs_file_t* new_socket_fd(task_t* task, struct pico_socket* pico_sock, int state) {
	struct socket* sock = (struct socket*)zmalloc(sizeof(struct socket));
	sock->pico_socket = pico_sock;
	sock->state = state;
	pico_sock->priv = (void*)sock;

	vfs_file_t* fd = vfs_alloc_fileno(task, 3);
	if(!fd) {
		return NULL;
	}

	fd->type = FT_IFSOCK;
	fd->flags = O_RDWR;
	fd->callbacks.read = vfs_read_cb;
	fd->callbacks.write = vfs_write_cb;
	fd->callbacks.poll = vfs_poll_cb;
	fd->mount_instance = (void*)sock;
	debug("new_socket_fd, set up %#x pico %#x\n", pico_sock->priv, pico_sock);
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
	vfs_file_t* fd = new_socket_fd(task, pico_sock, SOCK_OPEN);
	if(!fd) {
		return -1;
	}

	return fd->num;
}

int net_bind(task_t* task, int sockfd, const struct sockaddr* addr,
	socklen_t addrlen) {
	struct socket* sock = get_socket(task, sockfd);
	if(!sock) {
		return -1;
	}

	uint16_t port = net_bsd_to_pico_port(addr, addrlen);
	if(endian_swap16(port) <= 1024 && task->euid) {
		sc_errno = EACCES;
		return -1;
	}

	union pico_address pico_addr = { .ip4 = { 0 } };
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

int net_accept(task_t* task, int sockfd, struct sockaddr* oaddr,
	socklen_t* addrlen) {

	struct socket* sock = get_socket(task, sockfd);
	if(!sock) {
		return -1;
	}

	if(sock->state == SOCK_CONNECTED) {
		sc_errno = EBADF;
		return -1;
	}

	struct sockaddr* addr = NULL;
	if(oaddr && addrlen) {
		/* Since sockaddr is variable length and the length is passed in a pointer,
		 * we can't use the syscall system's automagic kernel memory mapping.
		 */
		vm_alloc_t alloc;
		addr = vm_map(VM_KERNEL, &alloc, &task->vmem, oaddr,
			*addrlen, VM_MAP_USER_ONLY | VM_RW);

		if(!addr) {
			task_signal(task, NULL, SIGSEGV);
			sc_errno = EFAULT;
			return -1;
		}
	}

	int_enable();
	while(!sock->conn_requests) {
		asm("hlt\n");
	}
	int_disable();

	if(!spinlock_get(&net_pico_lock, 200)) {
		// FIXME
		//vm_free(&alloc);

		sc_errno = EAGAIN;
		return -1;
	}

	union pico_address pico_addr;
	uint16_t port;
	struct pico_socket* pico_sock = pico_socket_accept(sock->pico_socket, &pico_addr, &port);
	spinlock_release(&net_pico_lock);

	if(addr) {
		net_conv_pico2bsd(addr, SOCKSIZE, &pico_addr, port);
	}

	// FIXME
	int yes = 1;
	pico_socket_setoption(pico_sock, PICO_TCP_NODELAY, &yes);

	if(!pico_sock) {
		debug("accept done, but no pico sock\n");
		// FIXME
		//vm_free(&alloc);
		sc_errno = pico_err;
		return -1;
	}

	vfs_file_t* new_fd = new_socket_fd(task, pico_sock, SOCK_CONNECTED);
	if(!new_fd) {
		return -1;
	}

	debug("accept %#x, pico %#x\n", pico_sock->priv, pico_sock);
	sock->conn_requests--;

	// FIXME
	//vm_free(&alloc);
	return new_fd->num;
}

int net_getpeername(task_t* task, int sockfd, struct sockaddr* osa,
	socklen_t* addrlen) {

	struct socket* sock = get_socket(task, sockfd);
	if(!sock) {
		return -1;
	}

	if(*addrlen < SOCKSIZE) {
		sc_errno = ENOBUFS;
		return -1;
	}

	/* Since sockaddr is variable length and the length is passed in a pointer,
	 * we can't use the syscall system's automagic kernel memory mapping.
	 */
	vm_alloc_t alloc;
	struct sockaddr* sa = vm_map(VM_KERNEL, &alloc, &task->vmem, osa,
		*addrlen, VM_MAP_USER_ONLY | VM_RW);

	if(!sa) {
		task_signal(task, NULL, SIGSEGV);
		sc_errno = EFAULT;
		return -1;
	}

	union pico_address addr;
	uint16_t port;
	uint16_t proto;
	if(pico_socket_getpeername(sock->pico_socket, &addr, &port, &proto) < 0) {
		sc_errno = pico_err;
		return -1;
	}

	int r = net_conv_pico2bsd(sa, SOCKSIZE, &addr, port);

	// FIXME
	//vm_free(&alloc);
	return r;
}

int net_getsockname(task_t* task, int sockfd, struct sockaddr* oaddr,
	socklen_t* addrlen) {

	struct socket* sock = get_socket(task, sockfd);
	if(!sock) {
		return -1;
	}

	/* Since sockaddr is variable length and the length is passed in a pointer,
	 * we can't use the syscall system's automagic kernel memory mapping.
	 */
	vm_alloc_t alloc;
	struct sockaddr* addr = vm_map(VM_KERNEL, &alloc, &task->vmem, oaddr,
		*addrlen, VM_MAP_USER_ONLY | VM_RW);

	if(!addr) {
		task_signal(task, NULL, SIGSEGV);
		sc_errno = EFAULT;
		return -1;
	}

	int r = net_conv_pico2bsd(addr, SOCKSIZE, &sock->pico_socket->local_addr,
		sock->pico_socket->local_port);

	// FIXME
	//vm_free(&alloc);
	return r;
}

struct dns_cb_state {
	char* dest;
	int dest_len;
	int result;
};

static void dns_cb(char* data, void* _state) {
	struct dns_cb_state* state = (struct dns_cb_state*)_state;

	// Not interested anymore
	if(state->result == -3) {
		kfree(state);
		return;
	}

	if(data) {
		strlcpy(state->dest, data, state->dest_len);
		//kfree(data);
		state->result = 0;
	} else {
		state->result = -1;
	}
}

static int do_resolve(task_t* task, const char* data, char* result, int result_len, int mode) {
	struct dns_cb_state* state = kmalloc(sizeof(struct dns_cb_state));
	state->result = -2;
	state->dest = result;
	state->dest_len = result_len;

	if((mode ? pico_dns_client_getaddr : pico_dns_client_getname)(data, dns_cb, state) != 0) {
		sc_errno = pico_err;
		return -1;
	}

	uint32_t end_tick = timer_tick + (5 * timer_rate);
	int_enable();
	while(timer_tick < end_tick && state->result == -2) {
		scheduler_yield();
	}
	int_disable();

	switch(state->result) {
		// Timeout - leave deallocation to callback
		case -2:
			state->result = -3;
			return -1;
		// Success
		case 0:
			kfree(state);
			return 0;
		// Resolution failure
		case -1:
		default:
			kfree(state);
			return -1;
	}
}

int net_getaddr(task_t* task, const char* host, char* result, int result_len) {
	return do_resolve(task, host, result, result_len, 1);
}

int net_getname(task_t* task, const char* ip, char* result, int result_len) {
	return do_resolve(task, ip, result, result_len, 0);
}

int net_connect(task_t* task, int sockfd, const struct sockaddr* sa, uint32_t addrlen) {
	struct socket* sock = get_socket(task, sockfd);
	if(!sock) {
		return -1;
	}

	if(sa->sa_family != AF_INET || addrlen != sizeof(struct sockaddr_in)) {
		sc_errno = EAFNOSUPPORT;
		return -1;
	}

	uint16_t port = net_bsd_to_pico_port(sa, addrlen);
	if(endian_swap16(port) <= 1024 && task->euid) {
		sc_errno = EACCES;
		return -1;
	}

	union pico_address pico_addr = { .ip4 = { 0 } };
	if(net_bsd_to_pico_addr(&pico_addr, sa, addrlen) < 0) {
		sc_errno = EINVAL;
		return -1;
	}

	if(!spinlock_get(&net_pico_lock, 200)) {
		sc_errno = EAGAIN;
		return -1;
	}

	if(pico_socket_connect(sock->pico_socket, &pico_addr, port) < 0) {
		spinlock_release(&net_pico_lock);
		sc_errno = pico_err;
		return -1;
	}

	spinlock_release(&net_pico_lock);
	sock->state = SOCK_CONNECTED;
	return 0;
}

#endif /* ENABLE_PICOTCP */
