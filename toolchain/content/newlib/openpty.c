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
#include <pty.h>
#include <fcntl.h>
#include <termios.h>

int openpty(int *amaster, int *aslave, char *name,
                   const struct termios *termp,
                   const struct winsize *winp) {

	*amaster = open("/dev/ptmx", O_RDWR | O_NOCTTY);
	if(*amaster < 0) {
		return -1;
	}

	if(ioctl(*amaster, TIOCGPTN, aslave) < 0) {
		return -1;
	}

	// FIXME handle termp and winp
	return 0;
}
