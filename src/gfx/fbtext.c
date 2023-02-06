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
#include <errno.h>
#include <string.h>
#include <log.h>
#include <bitmap.h>

#define PSF_FONT_MAGIC 0x864ab572
#define BOOT_LOGO_WIDTH 183
#define BOOT_LOGO_HEIGHT 60
#define BOOT_LOGO_PADDING 10
#define BOOT_LOGO_PADDING_BELOW 30

#define PIXEL_PTR(dbuf, x, y) 									\
	((uint32_t*)((uintptr_t)(dbuf)								\
		+ (y)*gfx_handle->ul_desc.pitch							\
		+ (x)*(gfx_handle->ul_desc.bpp / 8)))

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
static size_t text_fb_size;
static void* text_fb_addr;
static bool initialized = false;
static spinlock_t lock;

void fbtext_write_char(char chr) {
	if(!initialized) {
		return;
	}

	if(chr == '\n' || last_x + 1 >= cols) {
		if(unlikely(!spinlock_get(&lock, 200))) {
			return;
		}

		last_y++;
		last_x = 0;
		spinlock_release(&lock);

		if(chr == '\n') {
			return;
		}
	}

	last_x++;
	if(last_y >= rows) {
		if(unlikely(!spinlock_get(&lock, 200))) {
			return;
		}

		size_t move_size = gfx_handle->ul_desc.pitch * gfx_font.height;
		memcpy(text_fb_addr, text_fb_addr + move_size, text_fb_size - move_size);
		memset(text_fb_addr + text_fb_size - move_size, 0, move_size);
		last_y--;
		spinlock_release(&lock);
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
			*PIXEL_PTR(text_fb_addr, x + j, y + i) = fg ? 0xffffff : 0;
		}
	}
}

// Switch GFX output to fbtext. This is used during kernel panics
void gfx_fbtext_show() {
	if(!initialized) {
		return;
	}
	gfx_handle_enable(gfx_handle);
}

void gfx_fbtext_init() {
	gfx_handle = gfx_handle_init(VA_KERNEL);
	if(!gfx_handle) {
		log(LOG_ERR, "fbtext: Could not get gfx handle\n");
		return;
	}

	memset32(gfx_handle->ul_desc.addr, 0x000000, gfx_handle->ul_desc.size / 4);
	extern void* boot_logo;
	for(int i = 0; i < BOOT_LOGO_HEIGHT; i++) {
		memcpy(PIXEL_PTR(gfx_handle->ul_desc.addr, BOOT_LOGO_PADDING, i + BOOT_LOGO_PADDING), (uint32_t*)&boot_logo + i*BOOT_LOGO_WIDTH, BOOT_LOGO_WIDTH * 4);
	}

	size_t offset = BOOT_LOGO_HEIGHT + BOOT_LOGO_PADDING_BELOW;
	text_fb_addr = gfx_handle->ul_desc.addr + gfx_handle->ul_desc.pitch * offset;
	text_fb_size = gfx_handle->ul_desc.size - gfx_handle->ul_desc.pitch * offset;

	cols = gfx_handle->ul_desc.width / gfx_font.width;
	rows = (gfx_handle->ul_desc.height - offset) / gfx_font.height;

	log(LOG_DEBUG, "fbtext: font width %d/%d height %d/%d flags %d\n", gfx_font.width, cols, gfx_font.height, rows, gfx_font.flags);

	initialized = true;
	gfx_fbtext_show();
	log_dump();
}
