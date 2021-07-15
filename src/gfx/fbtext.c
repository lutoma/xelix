/* fbtext.c: Text drawing on linear frame buffers
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

#include <gfx/fbtext.h>
#include <gfx/gfx.h>
#include <mem/kmalloc.h>
#include <boot/multiboot.h>
#include <fs/sysfs.h>
#include <errno.h>
#include <string.h>
#include <log.h>
#include <bitmap.h>

#define PSF_FONT_MAGIC 0x864ab572

#define PIXEL_PTR(dbuf, x, y) 									\
	((uint32_t*)((uintptr_t)(dbuf)								\
		+ (y)*gfx_handle->pitch					\
		+ (x)*(gfx_handle->bpp / 8)))

#define CHAR_PTR(dbuf, x, y) PIXEL_PTR(dbuf, x * gfx_font.width, y * gfx_font.height)

int cols = 0;
int rows = 0;

// Font from ter-u16n.psf gets linked into the binary
extern struct {
	uint32_t magic;
	uint32_t version;
	uint32_t header_size;
	// 1 if unicode table exists, otherwise 0
	uint32_t flags;
	uint32_t num_glyphs;
	uint32_t bytes_per_glyph;
	uint32_t height;
	uint32_t width;
} gfx_font;

static struct gfx_handle* gfx_handle = NULL;
static unsigned int last_x = 0;
static unsigned int last_y = 0;
static bool initialized = false;

void fbtext_write_char(char chr) {
	if(!gfx_handle) {
		return;
	}

	if(chr == '\n' || last_x + 1 >= cols) {
		gfx_blit(gfx_handle, 0, last_y * gfx_font.height, gfx_handle->width, gfx_font.height);
		gfx_blit_all(gfx_handle);
		last_y++;
		last_x = 0;

		if(chr == '\n') {
			return;
		}
	}

	last_x++;
	if(last_y >= rows) {
		size_t move_size = gfx_handle->pitch * gfx_font.height;
		memcpy(gfx_handle->addr, gfx_handle->addr + move_size, gfx_handle->size - move_size);
		memset(gfx_handle->addr + gfx_handle->size - move_size, 0, move_size);
		gfx_blit_all(gfx_handle);
		last_y--;
	}

	unsigned int x = last_x * gfx_font.width;
	unsigned int y = last_y * gfx_font.height;

	const uint8_t* bitmap = (uint8_t*)&gfx_font
			+ gfx_font.header_size
			+ chr * gfx_font.bytes_per_glyph;

	if(unlikely(!bitmap)) {
		return;
	}

	for(int i = 0; i < gfx_font.height; i++) {
		for(int j = 0; j < gfx_font.width; j++) {
			int fg = bitmap[i] & (1 << (gfx_font.width - j - 1));
			*PIXEL_PTR(gfx_handle->addr, x + j, y + i) = fg ? 0xffffff : 0;
		}
	}
}

// Switch GFX output to fbtext. This is used during kernel panics
void gfx_fbtext_show() {
	if(!initialized) {
		return;
	}
	gfx_handle_enable(gfx_handle);
	gfx_blit_all(gfx_handle);
}

void gfx_fbtext_init() {
	gfx_handle = gfx_handle_init(NULL);
	if(!gfx_handle) {
		log(LOG_ERR, "fbtext: Could not get gfx handle\n");
		return;
	}

	cols = gfx_handle->width / gfx_font.width;
	rows = gfx_handle->height / gfx_font.height;

	log(LOG_DEBUG, "fbtext: font width %d/%d height %d/%d flags %d\n", gfx_font.width, cols, gfx_font.height, rows, gfx_font.flags);

	memset32(gfx_handle->addr, 0x000000, gfx_handle->size / 4);
	initialized = true;
	gfx_fbtext_show();
	log_dump();
}
