/* Copyright © 2019-2020 Lukas Martini
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <pty.h>

struct client {
	int fd;
	int pty_fd;
	struct sockaddr_in saddr;
	char ip_str[50];
	struct client* next;
};

struct client* clients = NULL;
struct pollfd* fds = NULL;
int server_fd = 0;
int n_clients = 0;
int n_fds = 0;

struct client* search_client(int fd) {
	struct client* client = clients;
	while(client) {
		if(client->fd == fd || client->pty_fd == fd) {
			return client;
		}
		client = client->next;
	}

	return NULL;
}

void rebuild_fds() {
	n_fds = (n_clients * 2) + 1;

	struct pollfd* new_fds = calloc(1, n_fds * sizeof(struct pollfd));
	new_fds[0].fd = server_fd;
	new_fds[0].events = POLLIN;

	struct client* client = clients;
	for(int i = 0; i < n_clients && client; i++) {
		new_fds[i*2 + 1].fd = client->fd;
		new_fds[i*2 + 1].events = POLLIN;
		new_fds[i*2 + 2].fd = client->pty_fd;
		new_fds[i*2 + 2].events = POLLIN;
		client = client->next;
	}

	free(fds);
	fds = new_fds;
}

struct client* accept_connection() {
	struct client* client = calloc(1, sizeof(struct client));

	socklen_t client_len = sizeof(client->saddr);
	client->fd = accept(server_fd, (struct sockaddr*)&client->saddr, &client_len);
	if (client->fd < 0) {
		perror("Could not establish new connection");
		free(client);
		return NULL;
	}

	if(getnameinfo((struct sockaddr*)&client->saddr, sizeof(client->saddr),
		client->ip_str, 50, NULL, 0, NI_NUMERICHOST) < 0) {
		perror("getnameinfo failed");
		free(client);
		return NULL;
	}

	struct winsize ws = {
		.ws_row = 25,
		.ws_col = 100,
		.ws_xpixel = 0,
		.ws_ypixel = 0,
	};

	char sname[PATH_MAX];
	pid_t pid = forkpty(&client->pty_fd, sname, NULL, &ws);
	if(pid < 0) {
		perror("forkpty failed");
		free(client);
		return NULL;
	}

	printf("Connection from %s:%d %s\n", client->ip_str,
		client->saddr.sin_port, sname);
	fflush(stdout);

	if(pid) {
	    int flags = fcntl(client->fd, F_GETFL);
	    fcntl(client->fd, F_SETFL, flags | O_NONBLOCK);
		return client;
	} else {
		close(server_fd);
		close(client->fd);
		free(client);

		char* login_argv[] = { "bash", "-i", NULL };
		char* login_env[] = { "USER=root", NULL };
		if(execve("/usr/bin/login", login_argv, login_env) < 0) {
			perror("execve failed");
			exit(EXIT_FAILURE);
		}
	}
}

int copy(int from, int to) {
	size_t bsize = getpagesize();
	char buf[bsize];
	int nread = read(from, buf, bsize);
	if(nread < 0 && errno != EAGAIN) {
		return -1;
	}

	if(nread > 0) {
		if(write(to, buf, nread) < 0) {
			return -1;
		}
	}
	return 0;
}

int main (int argc, char *argv[]) {
	struct sockaddr_in server;

	server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (server_fd < 0) {
		perror("Could not create socket");
		return -1;
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(23);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	int opt_val = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

	if(bind(server_fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
		perror("Could not bind socket");
		return -1;
	}

	if(listen(server_fd, 128) < 0) {
		perror("Could not listen on socket");
		return -1;
	}

	rebuild_fds();
	printf("Server is listening on port 23\n");
	fflush(stdout);

	while(1) {
		if(poll(fds, n_fds, -1) < 1) {
			continue;
		}

		for(int i = 0; i < n_fds; i++) {
			if(!(fds[i].revents & POLLIN)) {
				continue;
			}
			fds[i].revents = 0;

			int fd = fds[i].fd;
			if(fd == server_fd) {
				struct client* client = accept_connection();
				if(!client) {
					continue;
				}

				client->next = clients;
				clients = client;
				n_clients++;
				rebuild_fds();
			} else {
				struct client* client = search_client(fd);
				if(!client) {
					continue;
				}

				int copy_result = -1;
				if(fd == client->fd) {
					copy_result = copy(client->fd, client->pty_fd);
				} else if(fd == client->pty_fd) {
					copy_result = copy(client->pty_fd, client->fd);
				}

				if(copy_result < 0) {
					printf("connection to %s closed.\n", client->ip_str);
				}
			}
		}
	}

	return 0;
}
