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
#include <pty.h>
#include <fcntl.h>
#include <termios.h>
#include <limits.h>

int openpty(int* ptm, int* pts, char *name, const struct termios* termios,
	const struct winsize* winsize) {

	*ptm = open("/dev/ptmx", O_RDWR | O_NOCTTY);
	if(*ptm < 0) {
		return -1;
	}

	if(ioctl(*ptm, TIOCGPTN, pts) < 0) {
		close(ptm);
		return -1;
	}

	if(termios) {
		if(ioctl(*pts, TCSETS, termios) < 0) {
			close(ptm);
			return -1;
		}
	}

	if(winsize) {
		if(ioctl(*pts, TIOCSWINSZ, winsize) < 0) {
			close(ptm);
			return -1;
		}
	}

	if(name) {
		ttyname_r(*pts, name, PATH_MAX);
	}

	return 0;
}

pid_t forkpty(int* ptm, char* name, const struct termios* termios,
	const struct winsize* winsize) {

	int pts;
	if(openpty(ptm, &pts, name, termios, winsize) < 0) {
		return -1;
	}

	int pid = fork();
	if(pid < 0) {
		return -1;
	}

	if(pid) {
		close(pts);
		return pid;
	} else {
		close(ptm);

		// Map stdin/out to pts
		close(1);
		close(2);
		close(0);
		dup2(pts, 1);
		dup2(pts, 2);
		dup2(pts, 0);
		return 0;
	}
}
