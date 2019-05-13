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
#include <boot/multiboot.h>
#include <string.h>
#include <log.h>
#include <bitmap.h>

#define PSF_FONT_MAGIC 0x864ab572
#define PIXEL_PTR(x, y) ((uint32_t*)((uint32_t)framebuffer.addr \
	+ (y)*framebuffer.pitch + (x)*(framebuffer.bpp / 8)))
#define CHAR_PTR(x, y) PIXEL_PTR(x * tty_font.width, y * tty_font.height)

struct {
	uint32_t addr;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint8_t bpp;
} framebuffer;

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

#ifdef __arm__
#include <bsp/arm-mailbox.h>

/* A mailbox property request with multiple tags to set up a framebuffer.
 * Can't be sent using vc_prop_request as all the framebuffer setup needs
 * to be in the same request.
 */
#define mpr_tag(name, data_size) \
	struct { uint32_t id; uint32_t size; \
	uint32_t code; uint32_t data[data_size]; } name

struct mailbox_fb_request {
	uint32_t size;
	uint32_t code;
	mpr_tag(pres, 2);
	mpr_tag(vres, 2);
	mpr_tag(bpp, 1);
	mpr_tag(fb, 2);
	mpr_tag(pitch, 1);
	uint32_t end_tag;
} __aligned(16);
#endif

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

static uint32_t convert_color(int color, bool bg) {
	// RGB colors: Black, red, green, yellow, blue, magenta, cyan, white, default
	const uint32_t colors_fg[] = {0x272822, 0xf92672, 0xa6e22e, 0xe6db74,
		0x66d9ef, 0xfd5ff0, 0xa1efe4, 0xffffff, 0xe9e9e9};
	const uint32_t colors_bg[] = {0x272822, 0xf92672, 0xa6e22e, 0xe6db74,
		0x66d9ef, 0xfd5ff0, 0xa1efe4, 0xe9e9e9, 0x272822};

	if(color < 0 || color >= ARRAY_SIZE(colors_fg)) {
		color = 8;
	}
	return (bg ? colors_bg : colors_fg)[color];
}

static inline const uint8_t* get_char_bitmap(char chr, bool bdc) {
	if(likely(!bdc)) {
		return (uint8_t*)&tty_font
			+ tty_font.header_size
			+ chr * tty_font.bytes_per_glyph;
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

static void write_char(uint32_t x, uint32_t y, char chr, bool bdc, uint32_t fg_col, uint32_t bg_col) {
	x *= tty_font.width;
	y *= tty_font.height;

	const uint8_t* bitmap = get_char_bitmap(chr, bdc);
	if(unlikely(!bitmap)) {
		return;
	}

	const uint32_t fg_color = convert_color(fg_col, false);
	const uint32_t bg_color = convert_color(bg_col, true);

	for(int i = 0; i < tty_font.height; i++) {
		for(int j = 0; j < tty_font.width; j++) {
			int fg = bitmap[i] & (1 << (tty_font.width - j - 1));
			*PIXEL_PTR(x + j, y + i) = fg ? fg_color : bg_color;
		}
	}
}

static void clear(uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y) {
	size_t x_size = ((end_x - start_x) * tty_font.width * (framebuffer.bpp / 8));
	uint32_t color = convert_color(term->bg_color, true);

	if(start_x == 0 && end_x == drv->cols) {
		size_t y_size = ((end_y - start_y + 1)  * tty_font.height * framebuffer.pitch);
		memset32(CHAR_PTR(start_x, start_y), color, x_size + y_size);
		return;
	}

	for(int y = start_y; y <= end_y; y++) {
		for(int x = start_x; x <= end_x; x++) {
			// FIXME
			write_char(x, y, ' ', false, term->bg_color, term->bg_color);
		}
	}
}

static void scroll_line() {
	size_t size = framebuffer.width
		* framebuffer.height
		* (framebuffer.bpp / 8)
		- framebuffer.pitch;

	size_t offset = framebuffer.pitch * tty_font.height;
	memmove((void*)(intptr_t)framebuffer.addr,
		(void*)((intptr_t)framebuffer.addr + offset), size);

	clear(0, drv->rows - 1, drv->cols, drv->rows - 1);
}

static void set_cursor(uint32_t x, uint32_t y, bool restore) {
	x *= tty_font.width;
	y *= tty_font.height;

	static uint32_t* last_data = NULL;
	static uint32_t last_x = -1;
	static uint32_t last_y = -1;
	if(!last_data) {
		last_data = zmalloc(tty_font.height * sizeof(uint32_t));
	}

	if(restore && last_x != -1) {
		for(int i = 0; i < tty_font.height; i++) {
			*PIXEL_PTR(last_x, last_y + i) = last_data[i];
		}
	}


	for(int i = 0; i < tty_font.height; i++) {
		last_data[i] = *PIXEL_PTR(x, y + i);
	}


	last_x = x;
	last_y = y;
	for(int i = 0; i < tty_font.height; i++) {
		*PIXEL_PTR(x, y + i) = 0xfd971f;
	}
}

struct tty_driver* tty_fbtext_init() {
	#ifdef __i386__
	struct multiboot_tag_framebuffer* fb_desc = multiboot_get_framebuffer();
	if(!fb_desc) {
		return NULL;
	}

	framebuffer.addr = fb_desc->common.framebuffer_addr;
	framebuffer.pitch = fb_desc->common.framebuffer_pitch;
	framebuffer.width = fb_desc->common.framebuffer_width;
	framebuffer.height = fb_desc->common.framebuffer_height;
	framebuffer.bpp = fb_desc->common.framebuffer_bpp;
	#else /* ARM */

	// Get display resolution
	uint32_t* resolution = vc_prop_request(0x40003);
	if(!resolution) {
		log(LOG_ERR, "fbtext: Could not get preferred resolution\n");
		return NULL;
	}

	volatile __aligned(16) struct mailbox_fb_request fbreq = {
		.size = sizeof(fbreq),
		.code = 0,

		.pres  = {0x48003, sizeof(fbreq.pres.data),  0, {*resolution, *(resolution + 1)}},
		.vres  = {0x48004, sizeof(fbreq.vres.data),  0, {*resolution, *(resolution + 1)}},
		.bpp   = {0x48005, sizeof(fbreq.bpp.data),   0, {32}},
		.fb    = {0x40001, sizeof(fbreq.fb.data),    0, {16, 0}},
		.pitch = {0x40008, sizeof(fbreq.pitch.data), 0, {0}},

		.end_tag = 0,
	};

	vc_mbox_write(0x40000000 + (uint32_t)&fbreq, 8);
	vc_mbox_read(8);
	if(fbreq.code != (1 << 31)) {
 		log(LOG_ERR, "fbtext: Framebuffer mailbox call failed\n");
		return NULL;
	}

	framebuffer.width = fbreq.pres.data[0];
	framebuffer.height = fbreq.pres.data[1];
	framebuffer.bpp = fbreq.bpp.data[0];
	framebuffer.addr = fbreq.fb.data[0];
	framebuffer.pitch = fbreq.pitch.data[0];
	#endif

	if(tty_font.magic != PSF_FONT_MAGIC || !framebuffer.addr ||
		!framebuffer.width || !framebuffer.height || !framebuffer.bpp ||
		!framebuffer.pitch) {
		log(LOG_ERR, "fbtext: Initialization failed.\n");
		return NULL;
	}

	log(LOG_DEBUG, "fbtext: %dx%d bpp %d pitch 0x%x at 0x%x\n",
		framebuffer.width,
		framebuffer.height,
		framebuffer.bpp,
		framebuffer.pitch,
		(uint32_t)framebuffer.addr);

	log(LOG_DEBUG, "fbtext: font width %d height %d flags %d\n", tty_font.width, tty_font.height, tty_font.flags);

	drv = kmalloc(sizeof(struct tty_driver));
	drv->cols = framebuffer.width / tty_font.width;
	drv->rows = framebuffer.height / tty_font.height;
	drv->xpixel = framebuffer.width;
	drv->ypixel = framebuffer.height;
	drv->write = write_char;
	drv->scroll_line = scroll_line;
	drv->clear = clear;
	drv->set_cursor = set_cursor;

	clear(0, 0, drv->cols, drv->rows);
	return drv;
}
