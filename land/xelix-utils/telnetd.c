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

struct client {
	int fd;
	int stdout_pipe[2];
	int stdin_pipe[2];
	struct sockaddr_in saddr;
	char ip_str[50];
	struct client* next;
};

struct client* clients = NULL;
struct pollfd* fds = NULL;
int server_fd = 0;
int n_clients = 0;
int n_fds = 0;

static inline void nonblock(const int fileno) {
    int flags = fcntl(fileno, F_GETFL);
    fcntl(fileno, F_SETFL, flags | O_NONBLOCK);
}

struct client* search_client(int fd) {
	struct client* client = clients;
	while(client) {
		if(client->fd == fd || client->stdout_pipe[0] == fd) {
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
		new_fds[i*2 + 2].fd = client->stdout_pipe[0];
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

	getnameinfo((struct sockaddr*)&client->saddr, sizeof(client->saddr),
		client->ip_str, 50, NULL, 0, NI_NUMERICHOST);

	printf("Accepted connection from %s:%d\n", client->ip_str, client->saddr.sin_port);
	fflush(stdout);

	if(pipe(client->stdout_pipe) != 0 || pipe(client->stdin_pipe) != 0) {
		free(client);
		return NULL;
	}

	nonblock(client->stdout_pipe[0]);
	nonblock(client->fd);

	int pid = fork();
	if(!pid) {
		dup2(client->stdout_pipe[1], 1);
		dup2(client->stdout_pipe[1], 2);
		dup2(client->stdin_pipe[0], 0);

		char* login_argv[] = { "bash", "-i", NULL };
		char* login_env[] = { "USER=root", NULL };
		execve("/usr/bin/bash", login_argv, login_env);
	}

	return client;
}

int copy(int from, int to) {
	char buf[1024];
	int nread = read(from, buf, 1024);
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

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("Could not create socket");
		return -1;
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(23);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	int opt_val = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

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
					copy_result = copy(client->fd, client->stdin_pipe[1]);
				} else if(fd == client->stdout_pipe[0]) {
					copy_result = copy(client->stdout_pipe[0], client->fd);
				}

				if(copy_result < 0) {
					printf("connection to %s closed.\n", client->ip_str);
				}
			}
		}
	}

	return 0;
}
