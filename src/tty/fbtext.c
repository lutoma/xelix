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
#include <fs/sysfs.h>
#include <errno.h>
#include <string.h>
#include <log.h>
#include <bitmap.h>

#define PSF_FONT_MAGIC 0x864ab572

#define PIXEL_PTR(dbuf, x, y) 									\
	((uint32_t*)((uintptr_t)(dbuf)								\
		+ (y)*fb_desc->common.framebuffer_pitch					\
		+ (x)*(fb_desc->common.framebuffer_bpp / 8)))

#define CHAR_PTR(dbuf, x, y) PIXEL_PTR(dbuf, x * tty_font.width, y * tty_font.height)

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

static struct multiboot_tag_framebuffer* fb_desc;
static struct tty_driver* drv;

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

static inline uintptr_t get_fb_buf(struct terminal* term) {
	if(term == active_tty) {
		return fb_desc->common.framebuffer_addr;
	} else {
		return (uintptr_t)term->drv_buf;
	}

}

static void write_char(struct terminal* term, uint32_t x, uint32_t y, char chr, bool bdc) {
	if(drv->direct_access) {
		return;
	}

	x *= tty_font.width;
	y *= tty_font.height;

	const uint8_t* bitmap = get_char_bitmap(chr, bdc);
	if(unlikely(!bitmap)) {
		return;
	}

	uintptr_t dest = get_fb_buf(term);
	const uint32_t fg_color = convert_color(term->fg_color, false);
	const uint32_t bg_color = convert_color(term->bg_color, true);

	for(int i = 0; i < tty_font.height; i++) {
		for(int j = 0; j < tty_font.width; j++) {
			int fg = bitmap[i] & (1 << (tty_font.width - j - 1));
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
		size_t clear_size = lines * tty_font.height
			* fb_desc->common.framebuffer_pitch;

		memset32(CHAR_PTR(dest, start_x, start_y), color, clear_size / 4);
		return;
	}

	// Partial clear, do individual memset32 for each line
	int chars = MAX(1, end_x - start_x);
	size_t clear_size = chars * tty_font.width
		* (fb_desc->common.framebuffer_bpp / 8);

	for(int i = 0; i < lines * tty_font.height; i++) {
		void* mdest = PIXEL_PTR(dest, start_x * tty_font.width, start_y * tty_font.height + i);
		memset32(mdest, color, clear_size / 4);
	}
}

static void scroll_line(struct terminal* term) {
	if(drv->direct_access) {
		return;
	}

	size_t size = fb_desc->common.framebuffer_width
		* fb_desc->common.framebuffer_height
		* (fb_desc->common.framebuffer_bpp / 8)
		- fb_desc->common.framebuffer_pitch;

	size_t offset = fb_desc->common.framebuffer_pitch * tty_font.height;
	uintptr_t dest = get_fb_buf(term);

	memmove((void*)dest, (void*)(dest + offset), size);
	clear(term, 0, drv->rows - 1, drv->cols, drv->rows - 1);
	if(term->cursor_data.last_y >= tty_font.height) {
		term->cursor_data.last_y -= tty_font.height;
	}
}

static void set_cursor(struct terminal* term, uint32_t x, uint32_t y, bool restore) {
	if(drv->direct_access) {
		return;
	}

	uintptr_t dest = get_fb_buf(term);

	x *= tty_font.width;
	y *= tty_font.height;

	if(!term->cursor_data.last_data) {
		term->cursor_data.last_data = zmalloc(tty_font.height * sizeof(uint32_t));
	} else {
		if(restore) {
			for(int i = 0; i < tty_font.height; i++) {
				*PIXEL_PTR(dest, term->cursor_data.last_x, term->cursor_data.last_y + i) = term->cursor_data.last_data[i];
			}
		}
	}

	for(int i = 0; i < tty_font.height; i++) {
		term->cursor_data.last_data[i] = *PIXEL_PTR(dest, x, y + i);
	}

	term->cursor_data.last_x = x;
	term->cursor_data.last_y = y;
	for(int i = 0; i < tty_font.height; i++) {
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

	memcpy(tty_old->drv_buf, (void*)(uintptr_t)fb_desc->common.framebuffer_addr, tty_old->drv->buf_size);
	memcpy((void*)(uintptr_t)fb_desc->common.framebuffer_addr, tty_new->drv_buf, tty_new->drv->buf_size);
}

static int sfs_ioctl(struct vfs_callback_ctx* ctx, int request, void* _arg) {
	size_t size = fb_desc->common.framebuffer_width
		* fb_desc->common.framebuffer_height
		* fb_desc->common.framebuffer_bpp;

	switch(request) {
		case 0x2f01:
			return fb_desc->common.framebuffer_addr;
		case 0x2f02:
			return fb_desc->common.framebuffer_bpp;
		case 0x2f03:
			drv->direct_access = 1;
			vmem_map_flat(ctx->task->vmem_ctx,
				(void*)(uintptr_t)fb_desc->common.framebuffer_addr,
				size, VM_USER | VM_RW);

			return 0;
		case 0x2f04:
			drv->direct_access = 0;
			// FIXME unmap
			return 0;
		case 0x2f05:
			return fb_desc->common.framebuffer_width;
		case 0x2f06:
			return fb_desc->common.framebuffer_height;
		case 0x2f07:
			return fb_desc->common.framebuffer_pitch;
		default:
			sc_errno = ENOSYS;
			return -1;
	}
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

	log(LOG_DEBUG, "fbtext: font width %d height %d flags %d\n", tty_font.width, tty_font.height, tty_font.flags);

	// Map the framebuffer into the kernel paging context
	size_t vmem_size = fb_desc->common.framebuffer_width
		* fb_desc->common.framebuffer_height
		* fb_desc->common.framebuffer_bpp;
	vmem_map_flat(NULL, (void*)(uint32_t)fb_desc->common.framebuffer_addr, vmem_size, VM_RW);

	drv = kmalloc(sizeof(struct tty_driver));
	drv->cols = fb_desc->common.framebuffer_width / tty_font.width;
	drv->rows = fb_desc->common.framebuffer_height / tty_font.height;
	drv->xpixel = fb_desc->common.framebuffer_width;
	drv->ypixel = fb_desc->common.framebuffer_height;
	drv->buf_size = fb_desc->common.framebuffer_height * fb_desc->common.framebuffer_pitch;
	drv->write = write_char;
	drv->direct_access = 0;
	drv->scroll_line = scroll_line;
	drv->clear = clear;
	drv->set_cursor = set_cursor;
	drv->rerender = rerender;


	struct vfs_callbacks sfs_cb = {
		.ioctl = sfs_ioctl,
	};

	sysfs_add_dev("gfx1", &sfs_cb);
	return drv;
}
