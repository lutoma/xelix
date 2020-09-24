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
	cairo_surface_t* surface = win->decoration->cs;

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
	cairo_set_source(cr, pat);
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

void window_set_position(struct window* win, int32_t x, int32_t y) {
	win->decoration->x = x;
	win->decoration->y = y;
	win->surface->x = x + WINDOW_BORDER_SIZE;
	win->surface->y = y + TITLE_BAR_HEIGHT;
}

void window_move(struct window* win, int32_t x, int32_t y) {
	surface_move(win->decoration, x, y);
	surface_move(win->surface, x + WINDOW_BORDER_SIZE, y + TITLE_BAR_HEIGHT);
}

void decoration_mouse_handler(void* meta, uint32_t x, uint32_t y, struct mouse_event* ev) {
	struct window* win = (struct window*)meta;
	if(win && ev->button_left) {
		window_move(win, win->decoration->x + ev->x, win->decoration->y + ev->y);
	}
}

struct window* window_new(const char* title, size_t width, size_t height, uint32_t* data) {
	struct window* window = calloc(1, sizeof(struct window));
	if(!window) {
		return NULL;
	}

	window->id = __sync_add_and_fetch(&last_id, 1);
	window->buffer = data;
	window->width = width;
	window->height = height;

	memcpy(window->title, title, MIN(strlen(title), 1024));

	window->surface = surface_new(NULL, width, height);
	window->surface->name = "Window";
	window->decoration = surface_new(NULL, decoration_width(width), decoration_height(height));
	window->decoration->name = "Window decoration";
	window->decoration->mouse_ev_handler = decoration_mouse_handler;
	window->decoration->mouse_ev_meta = window;

	window_set_position(window, (gfx_handle.width + width) / 2, (gfx_handle.height + height) / 2);
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

void window_toggle_show(struct window* win, int show) {
	if(show == -1) {
		show = !win->surface->show;
	}

	win->surface->show = show;
	win->decoration->show = show;
	surface_blit_full(win->surface);
	surface_blit_full(win->decoration);
}

void window_blit(struct window* win, size_t width, size_t height, size_t x, size_t y) {
	memcpy((uint32_t*)cairo_image_surface_get_data(win->surface->cs), win->buffer,
		cairo_image_surface_get_height(win->surface->cs) * cairo_image_surface_get_stride(win->surface->cs));
	cairo_surface_mark_dirty(win->surface->cs);
	surface_blit(win->surface, width, height, x, y);
}

void window_add(struct window* win) {
	surface_add(win->decoration);
	surface_add(win->surface);
	surface_blit_full(win->decoration);

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

	active_window = win;
}

void window_init() {
	windows = NULL;
	active_window = NULL;
}
