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

void* main_buffer;
static cairo_surface_t* main_surface;
static cairo_surface_t* bg_surface_blurred;
static cairo_t* cr;
static uint32_t last_id = -1;

static inline void area_blit(size_t width, size_t height, size_t x, size_t y) {
	cairo_rectangle(cr, x, y, width, height);
	cairo_clip(cr);

	struct surface* surface = surfaces;
	for(; surface; surface = surface->next) {
		if(surface->show && surface_intersect(surface, width, height, x, y)) {
			cairo_surface_set_device_offset(surface->cs, x, y);
			cairo_set_source_surface(cr, surface->cs, surface->x + x, surface->y + y);
			cairo_paint(cr);
			cairo_surface_set_device_offset(surface->cs, 0, 0);
		}
	}

	cairo_reset_clip(cr);

	for(size_t cy = y; cy < y + height && cy < gfx_handle.height; cy++) {
		uintptr_t offset = cy * gfx_handle.pitch + (x * gfx_handle.bpp/8);
		memcpy((void*)((uintptr_t)gfx_handle.addr + offset), main_buffer + offset, width * gfx_handle.bpp/8);
	}
}

void surface_blit(struct surface* surface, size_t width, size_t height, size_t x, size_t y) {
	x = MIN(x, surface->width);
	y = MIN(y, surface->height);
	width = MIN(surface->width - x, width);
	height = MIN(surface->height - y, height);
	area_blit(width, height, surface->x + x, surface->y + y);
}

void surface_move(struct surface* surface, int32_t x, int32_t y) {
	int32_t old_x = surface->x;
	int32_t old_y = surface->y;
	surface->x = x;
	surface->y = y;
	area_blit(surface->width, surface->height, old_x, old_y);
	area_blit(surface->width, surface->height, surface->x, surface->y);
}

void surface_add(struct surface* surface) {
	struct surface* prev = surfaces;
	for(; prev; prev = prev->next) {
		if(prev->z <= surface->z && (!prev->next || prev->next->z >= surface->z)) {
			break;
		}
	}

	if(prev) {
		surface->next = prev->next;
		prev->next = surface;
	} else {
		surfaces = surface;
	}
}

struct surface* surface_new(cairo_surface_t* cs, size_t width, size_t height) {
	struct surface* surface = calloc(1, sizeof(struct surface));
	if(!surface) {
		return NULL;
	}

	surface->id = __sync_add_and_fetch(&last_id, 1);
	surface->show = true;
	surface->width = width;
	surface->height = height;
	surface->z = surface->id;
	surface->name = "unnamed";

	if(cs) {
		surface->cs = cs;
	} else {
		surface->cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
		if(!surface->cs) {
			free(surface);
			return NULL;
		}
	}

	return surface;
}

void render_init() {
	surfaces = NULL;

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

	main_buffer = malloc(gfx_handle.size);
	if(!main_buffer) {
		printf("Could not allocate main_buffer\n");
		exit(1);
	};

	main_surface = cairo_image_surface_create_for_data(
		main_buffer, CAIRO_FORMAT_ARGB32, gfx_handle.width,
		gfx_handle.height, gfx_handle.pitch);

	cr = cairo_create(main_surface);

	// Create background surface and fill with gray color
	cairo_surface_t* bg_surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, gfx_handle.width, gfx_handle.height);
	cairo_t* bg_cr = cairo_create(bg_surface);
	cairo_set_source_rgb(bg_cr, 0.2, 0.2, 0.2);
	cairo_paint(bg_cr);

	// If we have a wallpaper, fill it in as pattern
	cairo_surface_t* wallpaper = cairo_image_surface_create_from_png("/usr/share/gfxcompd/bg.png");
	if(wallpaper) {
		cairo_pattern_t* bg_pat = cairo_pattern_create_for_surface(wallpaper);
		cairo_pattern_set_extend(bg_pat, CAIRO_EXTEND_REPEAT);
		cairo_set_source(bg_cr, bg_pat);
		cairo_paint(bg_cr);
		cairo_surface_destroy(wallpaper);
		cairo_pattern_destroy(bg_pat);
	}

	cairo_destroy(bg_cr);

	struct surface* bgs = surface_new(bg_surface, gfx_handle.width, gfx_handle.height);
	bgs->z = -1000;
	bgs->name = "Background";
	surface_add(bgs);

/*	bg_surface_blurred = cairo_image_surface_create_from_png("/usr/share/gfxcompd/bg.png");

	cairo_surface_flush(bg_surface_blurred);
	iir_gauss_blur(
		cairo_image_surface_get_width(bg_surface_blurred),
		cairo_image_surface_get_height(bg_surface_blurred), 4,
		cairo_image_surface_get_data(bg_surface_blurred), 15);
	cairo_surface_mark_dirty(bg_surface_blurred);
*/

	cairo_set_source_surface(cr, bg_surface, 0, 0);
	cairo_paint(cr);

	// Enable gfx handle
	ioctl(gfx_fd, 0x2f02, gfx_handle.id);

	// Full-screen blit
	cairo_surface_flush(main_surface);
	memcpy((unsigned char*)gfx_handle.addr, main_buffer, gfx_handle.size);
}
