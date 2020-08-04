#pragma once

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

#include <mem/vmem.h>

struct gfx_handle {
	unsigned int id;
	bool used;

	struct vmem_context* ctx;
	void* addr;
	void* buf_addr;

	int bpp;
	int width;
	int height;
	int pitch;
	size_t size;
};

struct gfx_handle* gfx_get_handle(unsigned int id);
void gfx_handle_render(struct gfx_handle* handle);
void gfx_handle_enable(struct gfx_handle* handle);
struct gfx_handle* gfx_handle_init(struct vmem_context* ctx);
void gfx_init();
