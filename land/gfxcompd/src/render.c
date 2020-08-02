#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <cairo/cairo.h>

#include "util.h"
#include "render.h"
#include "window.h"
#include "mouse.h"

static cairo_surface_t* main_surface;
static cairo_surface_t* bg_surface;
static cairo_t* cr;
static bool initial_render_done = false;

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

	mouse_render_cursor(cr);
	cairo_surface_flush(main_surface);
	memcpy(fb.gfx_handle.addr, fb.buf, fb.gfx_handle.width * fb.gfx_handle.pitch);

	if(!initial_render_done) {
		// Display gfx handle
		ioctl(fb.fd, 0x2f02, fb.gfx_handle.id);
		initial_render_done = true;
	}
}

void render_init() {
	// Get framebuffer
	fb.fd = open("/dev/gfx1", O_WRONLY);
	if(!fb.fd) {
		perror("Could not open /dev/gfx1");
		exit(EXIT_FAILURE);
	}

	if(ioctl(fb.fd, 0x2f01, &fb.gfx_handle) < 0) {
		perror("Could not get GFX handle");
		exit(EXIT_FAILURE);
	}


	fb.buf = malloc(fb.gfx_handle.size);
	main_surface = cairo_image_surface_create_for_data(
		(unsigned char*)fb.buf, CAIRO_FORMAT_ARGB32, fb.gfx_handle.width,
		fb.gfx_handle.height, fb.gfx_handle.pitch);

	cr = cairo_create(main_surface);
	bg_surface = cairo_image_surface_create_from_png("/usr/share/gfxcompd/bg.png");
}
