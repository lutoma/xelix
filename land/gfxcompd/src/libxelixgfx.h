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

#ifndef LIBXELIXGFX_H
#define LIBXELIXGFX_H

#include <stdint.h>
#include <stddef.h>

#define gfx_window_blit_full(win) gfx_window_blit((win), 0, 0, (win)->width, (win)->height)

struct gfx_window {
	uint32_t* addr;
	uint32_t bpp;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t size;
	uint32_t wid;
};

int gfx_open();
int gfx_close();
int gfx_window_new(struct gfx_window* win, const char* title, size_t width, size_t height);
int gfx_window_blit(struct gfx_window* win, size_t x, size_t y, size_t width, size_t height);

#endif
