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

#ifndef _PTY_H
#define _PTY_H
#ifdef __cplusplus
extern "C" {
#endif

#include <termios.h>

#define TIOCGPTN 0x4016

int openpty(int* ptm, int* pts, char *name, const struct termios* termios,
	const struct winsize* winsize);

pid_t forkpty(int* ptm, char* name, const struct termios* termios,
	const struct winsize* winsize);

#ifdef __cplusplus
}       /* C++ */
#endif
#endif /* _PTY_H */
