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

// Map select() calls to poll syscall

#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <sys/select.h>

static inline void __convert_fd_set(struct pollfd* pfds, fd_set* set,
	short events, int* found, int max) {

	for(int i = 0; i < _howmany(FD_SETSIZE, NFDBITS) && *found < max; i++) {
		if(set->fds_bits[i]) {
			for(int j = 0; j < NFDBITS && *found < max; j++) {
				int bit = i * NFDBITS + j;

				if(FD_ISSET(bit, set)) {
					pfds[*found].fd = bit;
					pfds[*found].events = events;
					(*found)++;
				}
			}
		}
	}
}

int select(int nfds, fd_set *readfds, fd_set *writefds,
	fd_set *exceptfds, struct timeval *timeout) {

	struct pollfd* pfds = calloc(nfds, sizeof(struct pollfd));
	if(!pfds) {
		return -1;
	}

	int found = 0;
	if(readfds) {
		__convert_fd_set(pfds, readfds, POLLIN, &found, nfds);
		FD_ZERO(readfds);
	}

	if(writefds) {
		__convert_fd_set(pfds, writefds, POLLOUT, &found, nfds);
		FD_ZERO(writefds);
	}

	if(exceptfds) {
		FD_ZERO(exceptfds);
	}

	int r = poll(pfds, found, timeout ? timeout->tv_sec * 1000 : -1);
	if(r < 0) {
		free(pfds);
		return r;
	}

	for(int i = 0; i < found; i++) {
		if(readfds && pfds[i].revents & POLLIN) {
			FD_SET(pfds[i].fd, readfds);
		}
		if(writefds && pfds[i].revents & POLLOUT) {
			FD_SET(pfds[i].fd, writefds);
		}
		if(exceptfds && pfds[i].revents & (POLLERR | POLLHUP)) {
			FD_SET(pfds[i].fd, exceptfds);
		}
	}

	free(pfds);
	return r;
}
