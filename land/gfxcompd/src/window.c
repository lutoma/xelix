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
#define WINDOW_BORDER_SIZE 4

#define decoration_width(x) (x + WINDOW_BORDER_SIZE * 2)
#define decoration_height(y) (y + TITLE_BAR_HEIGHT + WINDOW_BORDER_SIZE)

static uint32_t last_id = -1;

static inline void draw_decoration(struct window* win, size_t width, size_t height) {
	cairo_surface_t* surface = win->decoration;

	// Window border
	cairo_t* cr = cairo_create(surface);
	cairo_set_source_rgb(cr, 0.192, 0.211, 0.231);
  	cairo_rectangle(cr, WINDOW_BORDER_SIZE / 2, WINDOW_BORDER_SIZE / 2,
  		width - WINDOW_BORDER_SIZE, height - WINDOW_BORDER_SIZE);

	cairo_set_line_width(cr, WINDOW_BORDER_SIZE);
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
	cairo_set_font_face(cr, font_light);
	cairo_set_font_size(cr, 14);

	cairo_text_extents_t extents;
	cairo_text_extents(cr, win->title, &extents);

	double x = width/2 - (extents.width/2 + extents.x_bearing);
	double y = TITLE_BAR_HEIGHT/2 - (extents.height/2 + extents.y_bearing);
	cairo_move_to(cr, x, y);
	cairo_show_text(cr, win->title);

	cairo_destroy(cr);
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
	win->x = x + WINDOW_BORDER_SIZE;
	win->y = y + TITLE_BAR_HEIGHT;
}

void window_init() {
	windows = NULL;
}
