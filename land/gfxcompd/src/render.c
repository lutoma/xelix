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
	memcpy(fb.addr, fb.buf, fb.width * fb.pitch);
}

void render_init() {
	// Get framebuffer
	fb.fd = open("/dev/gfx1", O_WRONLY);
	if(!fb.fd) {
		perror("Could not open /dev/gfx1");
		exit(EXIT_FAILURE);
	}

	fb.bpp = ioctl(fb.fd, 0x2f02, (uint32_t)0);
	fb.width = ioctl(fb.fd, 0x2f05, (uint32_t)0);
	fb.height = ioctl(fb.fd, 0x2f06, (uint32_t)0);
	fb.pitch = ioctl(fb.fd, 0x2f07, (uint32_t)0);
	fb.size = fb.height * fb.pitch;

	// Get framebuffer address
	fb.addr = (uint32_t*)ioctl(fb.fd, 0x2f01, (uint32_t)0);
	if(!fb.addr || !fb.size) {
		perror("Could not get memory mapping");
		exit(EXIT_FAILURE);
	}

	// Map framebuffer into our address space
	ioctl(fb.fd, 0x2f03, (uint32_t)0);

	fb.buf = malloc(fb.pitch * fb.width);
	main_surface = cairo_image_surface_create_for_data(
		(unsigned char*)fb.buf, CAIRO_FORMAT_ARGB32, fb.width, fb.height, fb.pitch);

	//main_surface = cairo_image_surface_create_for_data(
	//	(unsigned char*)fb.addr, CAIRO_FORMAT_ARGB32, fb.width, fb.height, fb.pitch);

	cr = cairo_create(main_surface);
	bg_surface = cairo_image_surface_create_from_png("/usr/share/gfxcompd/bg.png");
}
