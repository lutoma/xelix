#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <time.h>
#include <unistd.h>
#include <cairo/cairo.h>

#include "util.h"
#include "render.h"
#include "window.h"
#include "text.h"

static struct surface* bg_surface;
static struct surface* datetime_surface;
static struct surface* window_list_surface;

static inline void clear_cs(cairo_t* cr) {
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
}

static void render_datetime() {
	cairo_t* cr = cairo_create(datetime_surface->cs);
	clear_cs(cr);

    time_t timer = time(NULL);
    struct tm* tm_info = localtime(&timer);

    // Time
	char buffer[20];
    strftime(buffer, 26, "%H:%M:%S", tm_info);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_set_font_face(cr, font);
	cairo_set_font_size(cr, 15);

	cairo_text_extents_t extents;
	cairo_text_extents(cr, buffer, &extents);
	cairo_move_to(cr, datetime_surface->width - extents.width - 10, extents.height);
	cairo_show_text(cr, buffer);

	// Date
    strftime(buffer, 26, "%Y-%m-%d", tm_info);
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_set_font_face(cr, font_light);
	cairo_set_font_size(cr, 14);
	cairo_text_extents(cr, buffer, &extents);

	cairo_move_to(cr, datetime_surface->width - extents.width - 10, 35 - extents.height);
	cairo_show_text(cr, buffer);

	cairo_destroy(cr);
	surface_blit_full(datetime_surface);
}

void menu_button_mouse_cb(void* meta, uint32_t x, uint32_t y, struct mouse_event* ev) {
	if(ev->button_left) {
		int pid = fork();
		if(!pid) {
			execl("/usr/bin/gfxterm", "gfxterm");
			exit(-1);
		}
	}
}

static void render_menu_button() {
	cairo_surface_t* cs = cairo_image_surface_create_from_png("/usr/share/icons/view-app-grid-symbolic.png");
	struct surface* menu_button_surface = surface_new(cs, 25, 25);
	menu_button_surface->y = 5;
	menu_button_surface->x = 5;
	menu_button_surface->z = 401;
	menu_button_surface->name = "Bar menu button";
	menu_button_surface->mouse_ev_handler = menu_button_mouse_cb;
	surface_add(menu_button_surface);
	surface_blit_full(menu_button_surface);
}

static void render_window_list() {
	cairo_t* cr = cairo_create(window_list_surface->cs);
	clear_cs(cr);

	uint32_t xoff = 0;
	struct window* window = windows;
	for(; window; window = window->next) {

		cairo_text_extents_t extents;
		cairo_set_font_face(cr, font_light);
		cairo_set_font_size(cr, 14);
		cairo_text_extents(cr, window->title, &extents);

		size_t box_width = MAX(200, extents.width + extents.x_bearing + 16);

		// Draw background box
		cairo_rectangle(cr, xoff, 0, box_width, 35);
		cairo_clip(cr);

		if(window == active_window) {
			cairo_set_source_rgba(cr, 0.5, 0.9, 1.0, 0.3);
		} else {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.1);
		}

		cairo_paint(cr);
		cairo_reset_clip(cr);

		// Draw lower border
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
		cairo_move_to(cr, xoff, 33.5);
		cairo_line_to(cr, xoff + box_width, 33.5);
		cairo_set_line_width(cr, 1.5);
		cairo_stroke(cr);

		double y = 35/2 - (extents.height/2 + extents.y_bearing);
		cairo_move_to(cr, xoff + 8, y + 0.5);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_show_text(cr, window->title);

		xoff += box_width + 2;
	}

	cairo_destroy(cr);
	surface_blit_full(window_list_surface);
}

void window_list_mouse_cb(void* meta, uint32_t x, uint32_t y, struct mouse_event* ev) {
	if(ev->button_left) {
			struct window* window = windows;
			for(; window; window = window->next) {
				window_toggle_show(window, -1);
			}
	}
}


void update_bar() {
	render_datetime();
	render_window_list();
}

void bar_init() {
	bg_surface = surface_new(NULL, gfx_handle.width, 35);
	bg_surface->z = 400;
	bg_surface->name = "Bar background";
	surface_add(bg_surface);

	cairo_t* cr = cairo_create(bg_surface->cs);
	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, 256.0);
	cairo_pattern_add_color_stop_rgba(pat, 1, 0.231, 0.254, 0.278, 1);
	cairo_pattern_add_color_stop_rgba(pat, 0, 0.192, 0.211, 0.231, 1);
	cairo_set_source (cr, pat);
  	cairo_rectangle(cr, 0, 0, gfx_handle.width, 35);
  	cairo_fill(cr);
	cairo_pattern_destroy(pat);
	cairo_destroy(cr);

	surface_blit_full(bg_surface);

	datetime_surface = surface_new(NULL, 100, 25);
	datetime_surface->x = gfx_handle.width - 100;
	datetime_surface->y = 5;
	datetime_surface->z = 401;
	datetime_surface->name = "Bar datetime";
	surface_add(datetime_surface);

	window_list_surface = surface_new(NULL, gfx_handle.width - 145, 35);
	window_list_surface->x = 40;
	window_list_surface->y = 0;
	window_list_surface->z = 401;
	window_list_surface->mouse_ev_handler = window_list_mouse_cb;

	window_list_surface->name = "Bar window list";
	surface_add(window_list_surface);

	render_menu_button();
	update_bar();
}

