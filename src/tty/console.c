/* console.c: Xelix text console
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

#include <tty/term.h>
#include <tty/console.h>
#include <gfx/fbtext.h>
#include <mem/kmalloc.h>
#include <stdlib.h>
#include <panic.h>
#include <log.h>
#include "tmt.h"

struct cache_entry {
	wchar_t chr;
	uint32_t fg;
	uint32_t bg;
};

// RGBA colors: Black, red, green, yellow, blue, magenta, cyan, white, max
static const uint32_t colors[] = {0x00000000, 0xfffd6883, 0xffadda78, 0xfff9cc6c,
	0xffa8a9eb, 0xfff38d70, 0x85dacc, 0xfffff1f3, 0xffffffff};

TMT* vt;
size_t cursor_x = 0;
size_t cursor_y = 0;
static struct cache_entry* cache;
static uint32_t cache_pitch;


static size_t term_write_cb(struct term* term, const void* _source, size_t size) {
	char* source = (char*)_source;
	tmt_write(vt, source, size);
	return size;
}

static inline int set_cache(int col, int row, wchar_t chr, uint32_t fg, uint32_t bg) {
	struct cache_entry* ent = cache + col + row * cache_pitch;

	// Check for a match. Ignore foreground color for spaces.
	if(ent->chr == chr && ent->bg == bg &&
		(ent->chr == ' ' || ent->fg == fg)) {
		return 0;
	}

	ent->chr = chr;
	ent->fg = fg;
	ent->bg = bg;
	return 1;
}

static inline uint32_t normalize_color(int fg, int orig) {
	if(orig < 1 || orig > 8) {
		return fg ? 0xfffff1f3 : 0x00000000;
	}

	return colors[orig - 1];
}

		//serial_printf("do_clear, clearing %d at %d\n", empty_length, empty_start);
#define DO_CLEAR()                                                \
	if(empty_length) {                                            \
		gfx_fbtext_clear(empty_start, row, empty_length, 1);      \
		empty_length = 0;                                         \
	}

static inline void write_row(const TMTSCREEN* s, const TMTLINE* l, int row) {
	size_t empty_length = 0;
	unsigned int empty_start;

	for(size_t col = 0; col < s->ncol; col++) {
		const TMTCHAR* chr = &l->chars[col];

		// Normalize colors
		int fg = normalize_color(1, chr->a.fg);
		int bg = normalize_color(0, chr->a.bg);

		// Skip char if it's already on screen
		if(!set_cache(col, row, chr->c, fg, bg)) {
			DO_CLEAR();
			continue;
		}


		// Skip spaces so we can clear them as a block if there
		// are more contiguous to this one
		if(chr->c == ' ' && bg == 0) {
			if(empty_length == 0) {
				empty_start = col;
			}

			empty_length++;
		} else {
			DO_CLEAR();
			gfx_fbtext_write(col, row, chr->c, fg, bg);
		}
	}

	DO_CLEAR();
}

void tmt_callback(tmt_msg_t m, TMT *vt, const void *a, void *p) {
	const TMTSCREEN* s = tmt_screen(vt);
	const TMTPOINT* c = tmt_cursor(vt);

	switch(m) {
		case TMT_MSG_UPDATE:
			for(size_t row = 0; row < s->nline; row++) {
				if(s->lines[row]->dirty){
					write_row(s, s->lines[row], row);
				}
			}
			tmt_clean(vt);
			break;
		case TMT_MSG_ANSWER:
			term_input(term_console, a, strlen(a));
			break;
		case TMT_MSG_MOVED:
			// Redraw char at previous position
			struct cache_entry* ent = cache + cursor_x + cursor_y * cache_pitch;
			gfx_fbtext_write(cursor_x, cursor_y, ent->chr, ent->fg, ent->bg);

			cursor_x = c->c;
			cursor_y = c->r;
			gfx_fbtext_draw_cursor(cursor_x, cursor_y);
			break;
		default:
	}
}

void tty_console_init(int cols, int rows) {
	term_console = term_new("console", term_write_cb);
	term_console->winsize.ws_row = rows;
	term_console->winsize.ws_col = cols;

	cache = zmalloc(sizeof(struct cache_entry) * cols * rows);
	cache_pitch = cols;
	if(!cache) {
		panic("Could not allocate console cache.");
	}

	vt = tmt_open(rows, cols, tmt_callback, NULL, NULL);

	if(!vt) {
		panic("Could not open TMT virtual terminal.");
	}

	log_dump();
}
