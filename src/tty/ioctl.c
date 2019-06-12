/* ioctl.c: tty ioctl
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

#include <tty/ioctl.h>
#include <tty/tty.h>
#include <tasks/task.h>
#include <errno.h>
#include <stdlib.h>
#include <printf.h>

int tty_ioctl(const char* path, int request, void* arg, task_t* task) {
	struct terminal* term = tty_from_path(path, task);
	if(!term) {
		sc_errno = EINVAL;
		return -1;
	}

	switch(request) {
		case TIOCGWINSZ:;
			struct winsize* ws = (struct winsize*)arg;
			ws->ws_row = term->drv->rows;
			ws->ws_col = term->drv->cols;
			ws->ws_xpixel = term->drv->xpixel;
			ws->ws_ypixel = term->drv->ypixel;
			return 0;
		case TCGETS:
			memcpy(arg, &term->termios, sizeof(struct termios));
			return 0;
		case TCSETS:
		case TCSETSW:;
			memcpy(&term->termios, arg, sizeof(struct termios));
			return 0;
		default:
			sc_errno = ENOSYS;
			return -1;
	}
}

