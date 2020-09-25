/* Copyright © 2020 Lukas Martini
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

#include "libxelixgfx.h"
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

struct msg_window_new {
	uint32_t wid;
	void* addr;
	char title[1024];
	size_t width;
	size_t height;
	int32_t x;
	int32_t y;
};

struct msg_blit {
	uint32_t wid;
	size_t width;
	size_t height;
	int32_t x;
	int32_t y;
};

static int gfxbus_fd = -1;

int gfx_open() {
	// Get gfxbus handle
	gfxbus_fd = open("/dev/gfxbus", O_RDWR);
	if(!gfxbus_fd) {
		return -1;
	}

	return 0;
}

int gfx_close() {
	return close(gfxbus_fd);
}

int gfx_window_new(struct gfx_window* win, const char* title, size_t width, size_t height) {
	win->wid = ioctl(gfxbus_fd, 0x2f03);
	win->bpp = 32;
	win->width = width;
	win->height = height;
	win->pitch = win->width * 4;
	win->size = win->pitch * win->height * 4;
	win->addr = (uint32_t*)ioctl(gfxbus_fd, 0x2f02, win->size);

	struct msg_window_new msg = {
		.wid = win->wid,
		.addr = win->addr,
		.width = width,
		.height = height,
		.x = 50,
		.y = 100
	};

	strncpy(msg.title, title, 1023);

	int one = 1;
	write(gfxbus_fd, &one, 2);
	write(gfxbus_fd, &msg, sizeof(struct msg_window_new));
	return 0;
}

int gfx_window_blit(struct gfx_window* win, size_t x, size_t y, size_t width, size_t height) {
	uint16_t two = 2;
	struct msg_blit msg = {
		.wid = win->wid,
		.width = width,
		.height = height,
		.x = x,
		.y = y
	};

	write(gfxbus_fd, &two, 2);
	write(gfxbus_fd, &msg, sizeof(struct msg_blit));
	return 0;
}
