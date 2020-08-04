#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <cairo/cairo.h>

#include "util.h"
#include "render.h"
#include "window.h"
#include "mouse.h"
#include "text.h"

static cairo_surface_t* main_surface;
static cairo_surface_t* bg_surface;
static cairo_t* cr;

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
}

void render() {
	cairo_set_source_surface(cr, bg_surface, 0, 0);
	cairo_paint(cr);

	struct window* window = windows;
	for(; window; window = window->next) {
		if(!window->surface) {
			fprintf(serial, "Window %#x missing surface\n", window);
			continue;
		}

		cairo_set_source_surface(cr, window->surface, window->x, window->y);
		cairo_paint(cr);

		cairo_set_source_surface(cr, window->decoration, window->dx, window->dy);
		cairo_paint(cr);
	}

	render_bar(cr);
	mouse_render_cursor(cr);
	cairo_surface_flush(main_surface);

	// Request render
	ioctl(gfx_fd, 0x2f03, gfx_handle.id);
}

void render_init() {
	// Get framebuffer
	gfx_fd = open("/dev/gfx1", O_WRONLY);
	if(!gfx_fd) {
		perror("Could not open /dev/gfx1");
		exit(EXIT_FAILURE);
	}

	if(ioctl(gfx_fd, 0x2f01, &gfx_handle) < 0) {
		perror("Could not get GFX handle");
		exit(EXIT_FAILURE);
	}


	main_surface = cairo_image_surface_create_for_data(
		(unsigned char*)gfx_handle.addr, CAIRO_FORMAT_ARGB32, gfx_handle.width,
		gfx_handle.height, gfx_handle.pitch);

	cr = cairo_create(main_surface);
	bg_surface = cairo_image_surface_create_from_png("/usr/share/gfxcompd/bg.png");

		// Enable gfx handle
	ioctl(gfx_fd, 0x2f02, gfx_handle.id);
}
