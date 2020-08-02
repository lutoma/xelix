#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <math.h>

#include "util.h"
#include "render.h"
#include "window.h"
#include "text.h"

#define TITLE_BAR_HEIGHT 30
#define WINDoW_BORDER_SIZE 4

#define decoration_width(x) (x + WINDoW_BORDER_SIZE * 2)
#define decoration_height(y) (y + TITLE_BAR_HEIGHT + WINDoW_BORDER_SIZE)

static uint32_t last_id = -1;

static inline void draw_decoration(struct window* win, size_t width, size_t height) {
	cairo_surface_t* surface = win->decoration;

	// Window border
	cairo_t* cr = cairo_create(surface);
	cairo_set_source_rgb(cr, 0.192, 0.211, 0.231);
  	cairo_rectangle(cr, WINDoW_BORDER_SIZE / 2, WINDoW_BORDER_SIZE / 2,
  		width - WINDoW_BORDER_SIZE, height - WINDoW_BORDER_SIZE);

	cairo_set_line_width(cr, WINDoW_BORDER_SIZE);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
  	cairo_stroke(cr);

  	// Title bar
	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, 256.0);
	cairo_pattern_add_color_stop_rgba(pat, 1, 0.231, 0.254, 0.278, 1);
	cairo_pattern_add_color_stop_rgba(pat, 0, 0.192, 0.211, 0.231, 1);
	cairo_set_source (cr, pat);
  	cairo_rectangle(cr, 0, 0, width, TITLE_BAR_HEIGHT);
  	cairo_fill(cr);
	cairo_pattern_destroy (pat);


  	// Window title
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_set_font_face(cr, font);
	cairo_set_font_size(cr, 13);

	cairo_text_extents_t extents;
	cairo_text_extents(cr, win->title, &extents);

	double x = width/2 - (extents.width/2 + extents.x_bearing);
	double y = TITLE_BAR_HEIGHT/2 - (extents.height/2 + extents.y_bearing);
	cairo_move_to(cr, x, y);
	cairo_show_text(cr, win->title);

	cairo_destroy(cr);

	/*win->decoration = layer_new(width, height, NULL, 119);
	uint32_t* buf = win->decoration->data;

	uint32_t header_bg[] = {
		0xff3b4147, 0xff3a4046, 0xff3a4046, 0xff393f45, 0xff393f45,
		0xff383e44, 0xff383e43, 0xff383d43, 0xff373d42, 0xff373c42,
		0xff363c41, 0xff363b41, 0xff363b40, 0xff353b40, 0xff353a3f,
		0xff343a3f, 0xff34393e, 0xff33393e, 0xff33383d, 0xff33383d,
		0xff32373c, 0xff32373c, 0xff31363b, 0xff31363b, 0xff31363b,
		0xff31363b, 0xff31363b, 0xff31363b, 0xff31363b, 0xff31363b
	};

	for(int y = 0; y < height; y++) {
		uint32_t* row = buf + y * width;

		if(y < 30) {
			memset32(row, header_bg[y], width);
		} else if(y >= height - 4) {
			memset32(row, 0xff31363b, width);
		} else {
			*row = 0xff31363b;
			*(row + 1) = 0xff31363b;
			*(row + 2) = 0xff31363b;
			*(row + 3) = 0xff31363b;
			*(row + width - 4) = 0xff31363b;
			*(row + width - 3) = 0xff31363b;
			*(row + width - 2) = 0xff31363b;
			*(row + width - 1) = 0xff31363b;
		}
	}

	win->title_layer = text_to_layer(10, win->title);
	*/
}

struct window* window_new(const char* title, size_t width, size_t height, uint32_t* data) {
	struct window* window = calloc(1, sizeof(struct window));
	if(!window) {
		return NULL;
	}

	window->id = __sync_add_and_fetch(&last_id, 1);
	window->buffer = data;

	memcpy(window->title, title, MIN(strlen(title), 1024));

	window->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	window->decoration = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, decoration_width(width), decoration_height(height));
	draw_decoration(window, decoration_width(width), decoration_height(height));
	return window;
}

struct window* window_get(uint32_t id) {
	// FIXME use smarter storage method
	struct window* window = windows;
	for(; window; window = window->next) {
		if(window->id == id) {
			return window;
		}
	}

	return NULL;
}

void window_update(struct window* win) {
/*

	cairo_surface_flush(win->surface);
	int stride = cairo_image_surface_get_stride(win->surface);
	int height = cairo_image_surface_get_height(win->surface);
	memcpy((uint32_t*)cairo_image_surface_get_data(win->surface) + 30 * stride + 4 * 4, win->buffer,
		height * stride);
	cairo_surface_mark_dirty(win->surface);

*/

	cairo_surface_flush(win->surface);
	memcpy((uint32_t*)cairo_image_surface_get_data(win->surface), win->buffer,
		cairo_image_surface_get_height(win->surface) * cairo_image_surface_get_stride(win->surface));
	cairo_surface_mark_dirty(win->surface);
	//cairo_surface_mark_dirty(main_surface);
	//cairo_surface_flush(main_surface);
	//memcpy(fb.addr, fb.buf, fb.width * fb.pitch);
	//render();
}

void window_add(struct window* win) {
	fprintf(serial, "window_add %d\n", win->id);
	struct window* prev = windows;
	for(; prev; prev = prev->next) {
		if(prev->z <= win->z && (!prev->next || prev->next->z >= win->z)) {
			break;
		}
	}

	if(prev) {
		win->next = prev->next;
		prev->next = win;
	} else {
		windows = win;
	}
}

void window_set_position(struct window* win, int32_t x, int32_t y) {
	win->dx = x;
	win->dy = y;
	win->x = x + WINDoW_BORDER_SIZE;
	win->y = y + TITLE_BAR_HEIGHT;
}

void window_init() {
	windows = NULL;
}
