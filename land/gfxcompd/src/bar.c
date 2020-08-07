#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cairo/cairo.h>

#include "util.h"
#include "render.h"
#include "window.h"
#include "text.h"


void render_bar(cairo_t* cr) {
	cairo_pattern_t* pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, 256.0);
	cairo_pattern_add_color_stop_rgba(pat, 1, 0.231, 0.254, 0.278, 1);
	cairo_pattern_add_color_stop_rgba(pat, 0, 0.192, 0.211, 0.231, 1);
	cairo_set_source (cr, pat);
  	cairo_rectangle(cr, 0, 0, gfx_handle.width, 35);
  	cairo_fill(cr);
	cairo_pattern_destroy (pat);

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
	cairo_move_to(cr, gfx_handle.width - extents.width - 10, extents.height);
	cairo_show_text(cr, buffer);

	// Date
    strftime(buffer, 26, "%Y-%m-%d", tm_info);
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_set_font_face(cr, font_light);
	cairo_set_font_size(cr, 14);
	cairo_text_extents(cr, buffer, &extents);

	cairo_move_to(cr, gfx_handle.width - extents.width - 10, 35 - extents.height);
	cairo_show_text(cr, buffer);

	// Windows
	uint32_t xoff = 5;
	struct window* window = windows;
	for(; window; window = window->next) {
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_set_font_face(cr, font_light);
		cairo_set_font_size(cr, 13);
		cairo_text_extents(cr, window->title, &extents);


		double y = 35/2 - (extents.height/2 + extents.y_bearing);
		cairo_move_to(cr, xoff, y);
		cairo_show_text(cr, window->title);

		xoff += extents.width + extents.x_bearing + 5;
	}
/*
	struct gfx_ul_blit_cmd cmd = {
		.handle_id = gfx_handle.id,
		.x = 0,
		.y = 0,
		.width = gfx_handle.width,
		.height = 35
	};
	ioctl(gfx_fd, 0x2f04, &cmd);
*/
}
