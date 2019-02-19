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
#include <memory/kmalloc.h>
#include <multiboot.h>
#include <string.h>
#include <print.h>
#include <log.h>

#define PSF_FONT_MAGIC 0x864ab572

struct psf_font {
	uint32_t magic;
	uint32_t version;
	uint32_t header_size;
	// 1 if unicode table exists, otherwise 0
	uint32_t flags;
	uint32_t num_glyphs;
	uint32_t bytes_per_glyph;
	uint32_t height;
	uint32_t width;
};

// Font from ter-u16n.psf gets linked into the binary
extern struct psf_font _binary_src_tty_ter_u16n_psf_start;
extern char _binary_src_tty_ter_u16n_psf_end;

static struct multiboot_tag_framebuffer* fb_desc;
static struct psf_font* font = &_binary_src_tty_ter_u16n_psf_start;

static inline void set_pixel(uint32_t x, uint32_t y, uint32_t value) {
	uint32_t* pixel = (uint32_t*)((uint32_t)fb_desc->common.framebuffer_addr
		+ y*fb_desc->common.framebuffer_pitch
		+ x*(fb_desc->common.framebuffer_bpp / 8));

	*pixel = value;
}

static void write_char(uint32_t x, uint32_t y, char chr) {
	uint8_t* bitmap = (uint8_t*)font + font->header_size + (int)chr * font->bytes_per_glyph;
	for(int i = 0; i < font->height; i++) {
		uint8_t line = bitmap[i];
		for(int j = 0; j < font->width; j++) {
			if(bit_get(line, font->width - j)) {
				set_pixel(x * font->width + j, y * font->height + i, 0xdddddd);
			} else {
				set_pixel(x * font->width + j, y * font->height + i, 0);
			}
		}
	}
}

static void scroll_line() {
	size_t size = fb_desc->common.framebuffer_width
		* fb_desc->common.framebuffer_height
		* (fb_desc->common.framebuffer_bpp / 8)
		- fb_desc->common.framebuffer_pitch;

	size_t offset = fb_desc->common.framebuffer_pitch * font->height;
	memmove(fb_desc->common.framebuffer_addr, fb_desc->common.framebuffer_addr + offset, size);
	bzero(fb_desc->common.framebuffer_addr + size, offset);
}

struct tty_driver* tty_fbtext_init() {
	fb_desc = multiboot_get_framebuffer();
	if(!fb_desc || font->magic != PSF_FONT_MAGIC) {
		return NULL;
	}

	log(LOG_DEBUG, "fbtext: %dx%d bpp %d pitch 0x%x at 0x%x\n",
		fb_desc->common.framebuffer_width,
		fb_desc->common.framebuffer_height,
		fb_desc->common.framebuffer_bpp,
		fb_desc->common.framebuffer_pitch,
		(uint32_t)fb_desc->common.framebuffer_addr);

	struct tty_driver* drv = kmalloc(sizeof(struct tty_driver));
	drv->cols = fb_desc->common.framebuffer_width / font->width;
	drv->rows = fb_desc->common.framebuffer_height / font->height;
	drv->write = write_char;
	drv->scroll_line = scroll_line;
	return drv;
}
