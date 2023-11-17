/* fbtext.c: Text drawing on linear frame buffers
 * Copyright © 2019-2023 Lukas Martini
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
#include <tty/console.h>
#include <errno.h>
#include <string.h>
#include <log.h>
#include <bitmap.h>

#define PSF_FONT_MAGIC 0x864ab572
#define BOOT_LOGO_WIDTH 183
#define BOOT_LOGO_HEIGHT 60
#define BOOT_LOGO_PADDING 10
#define BOOT_LOGO_PADDING_BELOW 30

#define PIXEL_PTR(dbuf, x, y) 						\
	((uintptr_t)(dbuf)								\
		+ (y)*gfx_handle->ul_desc.pitch				\
		+ (x)*(gfx_handle->ul_desc.bpp / 8))

// Font from ter-u16n.psf gets linked into the binary
extern const struct {
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
static bool initialized = false;
static unsigned int font_width_aligned = 0;
static unsigned int font_width_bytes = 0;
static unsigned int font_height_pitch = 0;
static unsigned int pixel_bytes = 0;
static unsigned int num_cols = 0;
static unsigned int num_rows = 0;

static inline uint16_t color_convert16_565(int color) {
	uint16_t red = (color & 0xff0000) >> 16;
	uint16_t green = (color & 0xff00) >> 8;
	uint16_t blue =  color & 0xff;
	return (red >> 3 << 11) + (green >> 2 << 5) + (blue >> 3);
}

static inline void setpixel(uintptr_t ptr, uint32_t col) {
	if(gfx_handle->ul_desc.bpp == 32) {
		*(uint32_t*)ptr = col;
	} else if(gfx_handle->ul_desc.bpp == 16) {
		*(uint16_t*)ptr = (uint16_t)col;
	}
}

void gfx_fbtext_write(uint32_t x, uint32_t y, wchar_t chr, uint32_t col_fg, uint32_t col_bg) {
	if(unlikely(chr > gfx_font.num_glyphs)) {
		chr = 0;
	}

	uintptr_t cy_ptr = (uintptr_t)(gfx_handle->ul_desc.addr)
		+ y * font_height_pitch
		+ x * font_width_bytes;

	if(chr == ' ' && col_bg == 0) {
		for(int i = 0; i < gfx_font.height; i++) {
			memset((void*)cy_ptr, 0, font_width_bytes);
			cy_ptr += gfx_handle->ul_desc.pitch;
		}
		return;
	}

	if(gfx_handle->ul_desc.bpp == 16) {
		col_fg = color_convert16_565(col_fg);
		col_bg = color_convert16_565(col_bg);
	}

	const uint8_t* glyph = (const uint8_t*)&gfx_font
			+ gfx_font.header_size
			+ chr * gfx_font.bytes_per_glyph;

	int bit_offset = 0;
	unsigned int bit_index = 0;
	int bit_remainder = 7;

	for(uint32_t cy = 0; cy < gfx_font.height; cy++) {
		uintptr_t cx_ptr = cy_ptr;

		for(uint32_t cx = 0; cx < gfx_font.width; cx++) {
			if(bit_remainder < 0) {
				bit_index++;
				bit_remainder = 7;
			}

			int fg = bit_get(glyph[bit_index], bit_remainder);
			setpixel(cx_ptr, fg ? col_fg : col_bg);

			bit_remainder--;
			cx_ptr += pixel_bytes;
		}

		bit_offset += font_width_aligned;
		bit_index++;
		bit_remainder = 7;
		cy_ptr += gfx_handle->ul_desc.pitch;
	}
}

void gfx_fbtext_draw_cursor(uint32_t x, uint32_t y) {
	x *= gfx_font.width;
	y *= gfx_font.height;

	for(int i = 0; i < gfx_font.height; i++) {
		int color = 0xffffff;
		if(i == 0 || i == gfx_font.height - 1) {
			color = 0x000000;
		}

		setpixel(PIXEL_PTR(gfx_handle->ul_desc.addr, x, y + i), color);
	}
}

void gfx_fbtext_clear(uint32_t x, uint32_t y, uint32_t cols, uint32_t rows) {
	size_t clear_size = font_width_bytes * cols;
	uintptr_t offset = (uintptr_t)(gfx_handle->ul_desc.addr)
		+ y * font_height_pitch
		+ x * font_width_bytes;

	for(size_t i = 0; i < gfx_font.height * rows; i++) {
		memset((void*)offset, 0, clear_size);
		offset += gfx_handle->ul_desc.pitch;
	}
}

void gfx_fbtext_scroll(unsigned int num) {
	size_t lines = num * font_height_pitch;
	size_t size = gfx_handle->ul_desc.size - lines;
	void* from_ptr = gfx_handle->ul_desc.addr + lines;
	memmove(gfx_handle->ul_desc.addr, from_ptr, size);
	gfx_fbtext_clear(0, num_rows - 1, num_cols, 1);
}

// Switch GFX output to fbtext. This is used during kernel panics
void gfx_fbtext_show(void) {
	if(!initialized) {
		return;
	}
	gfx_handle_enable(gfx_handle);
}

void gfx_fbtext_init(void) {
	gfx_handle = gfx_handle_init(VM_KERNEL);
	if(!gfx_handle) {
		log(LOG_ERR, "fbtext: Could not get gfx handle\n");
		return;
	}

	if(gfx_handle->ul_desc.bpp != 32 && gfx_handle->ul_desc.bpp != 16) {
		log(LOG_ERR, "fbtext: Unsupported framebufer depth %d\n",
			gfx_handle->ul_desc.bpp);
		return;
	}

	//memset(gfx_handle->ul_desc.addr, 0, gfx_handle->ul_desc.size);
	num_cols = gfx_handle->ul_desc.width / gfx_font.width;
	num_rows = gfx_handle->ul_desc.height / gfx_font.height;

	log(LOG_DEBUG, "fbtext: font size %ux%u cols/rows %ux%u flags %u\n",
		gfx_font.width, gfx_font.height, num_cols, num_rows, gfx_font.flags);

	font_width_aligned = ALIGN(gfx_font.width, 8);
	font_height_pitch = gfx_font.height * gfx_handle->ul_desc.pitch;
	pixel_bytes = gfx_handle->ul_desc.bpp / 8;
	font_width_bytes = pixel_bytes * gfx_font.width;
	initialized = true;

	tty_console_init(num_cols, num_rows);
	gfx_fbtext_show();
}
