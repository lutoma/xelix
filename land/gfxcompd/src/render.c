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

#define IIR_GAUSS_BLUR_IMPLEMENTATION
#include "blur.h"

static cairo_surface_t* main_surface;
static cairo_surface_t* bg_surface;
static cairo_surface_t* bg_surface_blurred;
static cairo_t* cr;
static struct surface* surfaces = NULL;
static uint32_t last_id = -1;

struct surface* surface_new(size_t width, size_t height) {
	fprintf(serial, "surface_new %d x %d\n", width, height);
	struct surface* surface = calloc(1, sizeof(struct surface));
	if(!surface) {
		return NULL;
	}

	surface->id = __sync_add_and_fetch(&last_id, 1);
	surface->width = width;
	surface->height = height;

	surface->cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	if(!surface->cs) {
		free(surface);
		return NULL;
	}

	return surface;
}

/*
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
	//ioctl(gfx_fd, 0x2f03, gfx_handle.id);
}
*/

static inline void draw_surface_offset(cairo_surface_t* cs, size_t width, size_t height, size_t x, size_t y, size_t rx, size_t ry) {
	cairo_surface_set_device_offset(cs, x, y);

	// Clip destination surface
	cairo_rectangle(cr, rx, ry, width, height);
	cairo_clip(cr);

	// Copy
	cairo_set_source_surface(cr, cs, rx, ry);
	cairo_paint(cr);
	cairo_reset_clip(cr);
}

void surface_blit(struct surface* surface, size_t width, size_t height, size_t x, size_t y) {
	x = MIN(x, surface->width);
	y = MIN(y, surface->height);
	width = MIN(surface->width - x, width);
	height = MIN(surface->height - y, height);

	size_t rx = surface->x + x;
	size_t ry = surface->y + y;


	draw_surface_offset(bg_surface_blurred, width, height, x, y, rx, ry);
	draw_surface_offset(surface->cs, width, height, x, y, rx, ry);

	struct gfx_ul_blit_cmd cmd = {
		.handle_id = gfx_handle.id,
		.x = rx,
		.y = ry,
		.width = surface->width,
		.height = surface->height
	};
	ioctl(gfx_fd, 0x2f04, &cmd);
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
	bg_surface_blurred = cairo_image_surface_create_from_png("/usr/share/gfxcompd/bg.png");

	cairo_surface_flush(bg_surface_blurred);
	iir_gauss_blur(
		cairo_image_surface_get_width(bg_surface_blurred),
		cairo_image_surface_get_height(bg_surface_blurred), 4,
		cairo_image_surface_get_data(bg_surface_blurred), 10);

	cairo_surface_mark_dirty(bg_surface_blurred);


	cairo_set_source_surface(cr, bg_surface, 0, 0);
	cairo_paint(cr);
	cairo_surface_flush(main_surface);

	render_bar(cr);
	mouse_render_cursor(cr);

	// Enable gfx handle
	ioctl(gfx_fd, 0x2f02, gfx_handle.id);
	ioctl(gfx_fd, 0x2f03, gfx_handle.id);
}
