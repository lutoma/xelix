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

#include <tty/fbtext.h>
#include <tty/tty.h>
#include <mem/kmalloc.h>
#include <multiboot.h>
#include <string.h>
#include <print.h>
#include <log.h>
#include <bitmap.h>

#define PSF_FONT_MAGIC 0x864ab572

#define PIXEL_PTR(x, y) 										\
	((uint32_t*)((uint32_t)fb_desc->common.framebuffer_addr		\
		+ (y)*fb_desc->common.framebuffer_pitch					\
		+ (x)*(fb_desc->common.framebuffer_bpp / 8)))

#define CHAR_PTR(x, y) PIXEL_PTR(x * tty_font.width, y * tty_font.height)

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
} tty_font;

static struct multiboot_tag_framebuffer* fb_desc;
static struct tty_driver* drv;

static uint32_t convert_color(int color) {
	switch(color) {
		case 0: // black
			return 0x272822;
		case 1: // red
			return 0xF92672;
		case 2: // green
			return 0xA6E22E;
		case 3: // yellow
			return 0xE6DB74;
		case 4: // blue
			return 0x66D9EF;
		case 5: // magenta
			return 0xFD5FF0;
		case 6: // cyan
			return 0xA1EFE4;
		case 7: // white
		case 9: // default
		default:
			return 0xe9e9e9;
	}
}

static void write_char(uint32_t x, uint32_t y, char chr, uint32_t fg_col, uint32_t bg_col) {
	x *= tty_font.width;
	y *= tty_font.height;

	uint8_t* bitmap = (uint8_t*)&tty_font + tty_font.header_size + (int)chr * tty_font.bytes_per_glyph;
	uint32_t fg_color = convert_color(fg_col);
	uint32_t bg_color = convert_color(bg_col);

	for(int i = 0; i < tty_font.height; i++) {
		for(int j = 0; j < tty_font.width; j++) {
			int fg = bit_get(bitmap[i], tty_font.width - j);
			*PIXEL_PTR(x + j, y + i) = fg ? fg_color : bg_color;
		}
	}
}

static void clear(uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y) {
	uint32_t* start = CHAR_PTR(start_x, start_y);
	uint32_t* end = CHAR_PTR(end_x, end_y);
	uint32_t color = convert_color(term->bg_color);

	for(; start < end; start++) {
		*start = color;
	}
}

static void scroll_line() {
	size_t size = fb_desc->common.framebuffer_width
		* fb_desc->common.framebuffer_height
		* (fb_desc->common.framebuffer_bpp / 8)
		- fb_desc->common.framebuffer_pitch;

	size_t offset = fb_desc->common.framebuffer_pitch * tty_font.height;
	memmove((void*)(intptr_t)fb_desc->common.framebuffer_addr,
		(void*)((intptr_t)fb_desc->common.framebuffer_addr + offset), size);

	clear(drv->rows - 1, 0, drv->rows, drv->cols);
}

struct tty_driver* tty_fbtext_init() {
	fb_desc = multiboot_get_framebuffer();
	if(!fb_desc || tty_font.magic != PSF_FONT_MAGIC) {
		return NULL;
	}

	log(LOG_DEBUG, "fbtext: %dx%d bpp %d pitch 0x%x at 0x%x\n",
		fb_desc->common.framebuffer_width,
		fb_desc->common.framebuffer_height,
		fb_desc->common.framebuffer_bpp,
		fb_desc->common.framebuffer_pitch,
		(uint32_t)fb_desc->common.framebuffer_addr);


	drv = kmalloc(sizeof(struct tty_driver));
	drv->cols = fb_desc->common.framebuffer_width / tty_font.width;
	drv->rows = fb_desc->common.framebuffer_height / tty_font.height;
	drv->xpixel = fb_desc->common.framebuffer_width;
	drv->ypixel = fb_desc->common.framebuffer_height;
	drv->write = write_char;
	drv->scroll_line = scroll_line;
	drv->clear = clear;

	clear(0, 0, drv->cols, drv->rows);
	return drv;
}
