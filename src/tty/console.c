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
#include "tmt.h"

struct cache_entry {
	wchar_t chr;
	int fg;
	int bg;
};

TMT* vt;
static struct cache_entry* cache;
static uint32_t cache_pitch;

static inline uint32_t convert_color(int color, int bg) {
	if(color < 1 || color > 8) {
		return (bg ? 0x00000000 : 0xfffff1f3);
	}

	// RGBA colors: Black, red, green, yellow, blue, magenta, cyan, white, max
	const uint32_t colors[] = {0x00000000, 0xfffd6883, 0xffadda78, 0xfff9cc6c,
		0xffa8a9eb, 0xfff38d70, 0x85dacc, 0xfffff1f3, 0xffffffff};
	return colors[color - 1];
}

static size_t term_write_cb(struct term* term, const void* _source, size_t size) {
	char* source = (char*)_source;
	tmt_write(vt, source, size);
	return size;
}

static inline void write_char(int col, int row, TMTCHAR chr) {
	struct cache_entry* ent = cache + col + row * cache_pitch;
	if(ent->chr == chr.c && ent->fg == chr.a.fg && ent->bg == chr.a.bg) {
		return;
	}

	uint32_t fg = convert_color(chr.a.fg, 0);
	uint32_t bg = convert_color(chr.a.bg, 1);
	gfx_fbtext_write(col, row, chr.c, fg, bg);

	ent->chr = chr.c;
	ent->fg = chr.a.fg;
	ent->bg = chr.a.bg;
}

void tmt_callback(tmt_msg_t m, TMT *vt, const void *a, void *p) {
	const TMTSCREEN* s = tmt_screen(vt);
	const TMTPOINT* c = tmt_cursor(vt);

	switch(m) {
		case TMT_MSG_UPDATE:
			for (size_t row = 0; row < s->nline; row++){
				if (s->lines[row]->dirty){
					for (size_t col = 0; col < s->ncol; col++) {
						TMTCHAR chr = s->lines[row]->chars[col];
						write_char(col, row, chr);
					}
				}
			}
			tmt_clean(vt);
			break;
		case TMT_MSG_ANSWER:
			term_input(term_console, a, strlen(a));
			break;
		case TMT_MSG_MOVED:
			gfx_fbtext_set_cursor(c->c, c->r, true);
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
}
