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
#include <tty/tty.h>
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

// Special block drawing characters - Should get merged into main font
const uint8_t tty_fbtext_bdc_font[][16] = {
	{0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0xf0,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08},
	{0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x07,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0xff,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0f,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08},
	{0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0xf8,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08},
	{0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0xff,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08},
	{0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}
};

static struct tty_driver* drv;
static struct gfx_handle* gfx_handle;

static uint32_t convert_color(int color, bool bg) {
	// RGB colors: Black, red, green, yellow, blue, magenta, cyan, white, default
	const uint32_t colors_fg[] = {0x1e1e1e, 0xff453a, 0x32d74b, 0xffd60a,
		0x0a84ff, 0xbf5af2, 0x5ac8fa, 0xdedede, 0xffffff};
	const uint32_t colors_bg[] = {0x1e1e1e, 0xff453a, 0x32d74b, 0xffd60a,
		0x0a84ff, 0xbf5af2, 0x5ac8fa, 0xffffff, 0x1e1e1e};

	if(color < 0 || color >= ARRAY_SIZE(colors_fg)) {
		color = 8;
	}
	return (bg ? colors_bg : colors_fg)[color];
}

static inline const uint8_t* get_char_bitmap(char chr, bool bdc) {
	if(likely(!bdc)) {
		return (uint8_t*)&gfx_font
			+ gfx_font.header_size
			+ chr * gfx_font.bytes_per_glyph;
	}

	if(chr >= 'j' && chr <= 'n') {
		return tty_fbtext_bdc_font[chr - 'j'];
	}
	if(chr == 'q') {
		return tty_fbtext_bdc_font[5];
	}
	if(chr >= 't' && chr <= 'x') {
		return tty_fbtext_bdc_font[chr - 't' + 6];
	}
	return NULL;
}

static inline uintptr_t get_fb_buf(struct terminal* term) {
	if(term == active_tty) {
		return gfx_handle->addr;
	} else {
		return (uintptr_t)term->drv_buf;
	}

}

static void write_char(struct terminal* term, uint32_t x, uint32_t y, char chr, bool bdc) {
	if(drv->direct_access) {
		return;
	}

	x *= gfx_font.width;
	y *= gfx_font.height;

	const uint8_t* bitmap = get_char_bitmap(chr, bdc);
	if(unlikely(!bitmap)) {
		return;
	}

	uintptr_t dest = get_fb_buf(term);
	const uint32_t fg_color = convert_color(term->fg_color, false);
	const uint32_t bg_color = convert_color(term->bg_color, true);

	for(int i = 0; i < gfx_font.height; i++) {
		for(int j = 0; j < gfx_font.width; j++) {
			int fg = bitmap[i] & (1 << (gfx_font.width - j - 1));
			*PIXEL_PTR(dest, x + j, y + i) = fg ? fg_color : bg_color;
		}
	}
}

static void clear(struct terminal* term, uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y) {
	if(drv->direct_access) {
		return;
	}

	uint32_t color = convert_color(term->bg_color, true);
	uintptr_t dest = get_fb_buf(term);

	// If end_y == start_y, we still need to clear 1 line
	int lines = MAX(1, (end_y - start_y));

	// memset32 entire area for full line clears
	if(start_x == 0 && end_x == drv->cols) {
		size_t clear_size = lines * gfx_font.height
			* gfx_handle->pitch;

		memset32(CHAR_PTR(dest, start_x, start_y), color, clear_size / 4);
		return;
	}

	// Partial clear, do individual memset32 for each line
	int chars = MAX(1, end_x - start_x);
	size_t clear_size = chars * gfx_font.width
		* (gfx_handle->bpp / 8);

	for(int i = 0; i < lines * gfx_font.height; i++) {
		void* mdest = PIXEL_PTR(dest, start_x * gfx_font.width, start_y * gfx_font.height + i);
		memset32(mdest, color, clear_size / 4);
	}
}

static void scroll_line(struct terminal* term) {
	if(drv->direct_access) {
		return;
	}

	size_t size = gfx_handle->width
		* gfx_handle->height
		* (gfx_handle->bpp / 8)
		- gfx_handle->pitch;

	size_t offset = gfx_handle->pitch * gfx_font.height;
	uintptr_t dest = get_fb_buf(term);

	memmove((void*)dest, (void*)(dest + offset), size);
	clear(term, 0, drv->rows - 1, drv->cols, drv->rows - 1);
	if(term->cursor_data.last_y >= gfx_font.height) {
		term->cursor_data.last_y -= gfx_font.height;
	}
}

static void set_cursor(struct terminal* term, uint32_t x, uint32_t y, bool restore) {
	if(drv->direct_access) {
		return;
	}

	uintptr_t dest = get_fb_buf(term);

	x *= gfx_font.width;
	y *= gfx_font.height;

	if(!term->cursor_data.last_data) {
		term->cursor_data.last_data = zmalloc(gfx_font.height * sizeof(uint32_t));
	} else {
		if(restore) {
			for(int i = 0; i < gfx_font.height; i++) {
				*PIXEL_PTR(dest, term->cursor_data.last_x, term->cursor_data.last_y + i) = term->cursor_data.last_data[i];
			}
		}
	}

	for(int i = 0; i < gfx_font.height; i++) {
		term->cursor_data.last_data[i] = *PIXEL_PTR(dest, x, y + i);
	}

	term->cursor_data.last_x = x;
	term->cursor_data.last_y = y;
	for(int i = 0; i < gfx_font.height; i++) {
		*PIXEL_PTR(dest, x, y + i) = 0xfd971f;
	}
}

static void rerender(struct terminal* tty_old, struct terminal* tty_new) {
	if(drv->direct_access) {
		return;
	}

	if(!tty_old->drv_buf || !tty_new->drv_buf) {
		return;
	}

	memcpy(tty_old->drv_buf, (void*)(uintptr_t)gfx_handle->addr, tty_old->drv->buf_size);
	memcpy((void*)(uintptr_t)gfx_handle->addr, tty_new->drv_buf, tty_new->drv->buf_size);
}


struct tty_driver* gfx_fbtext_init() {
	gfx_handle = gfx_handle_init(NULL);
	if(!gfx_handle) {
		log(LOG_ERR, "fbtext: Could not get gfx handle\n");
		return NULL;
	}

	log(LOG_DEBUG, "fbtext: font width %d height %d flags %d\n", gfx_font.width, gfx_font.height, gfx_font.flags);
	memset32(gfx_handle->addr, 0x0000ff, gfx_handle->size / 4);

	drv = kmalloc(sizeof(struct tty_driver));
	drv->cols = gfx_handle->width / gfx_font.width;
	drv->rows = gfx_handle->height / gfx_font.height;
	drv->xpixel = gfx_handle->width;
	drv->ypixel = gfx_handle->height;
	drv->buf_size = gfx_handle->height * gfx_handle->pitch;
	drv->write = write_char;
	drv->direct_access = 0;
	drv->scroll_line = scroll_line;
	drv->clear = clear;
	drv->set_cursor = set_cursor;
	drv->rerender = rerender;

	gfx_handle_enable(gfx_handle->id);
	return drv;
}
