#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "mouse.h"

int gfx_fd;

struct {
	unsigned int id;
	bool used;

	uintptr_t _kernel_int;
	void* addr;
	uintptr_t _kernel_int2;

	int bpp;
	int width;
	int height;
	int pitch;
	size_t size;
} gfx_handle;

struct surface {
	uint32_t id;
	struct surface* next;
	struct surface* prev;
	cairo_surface_t* cs;

	size_t width;
	size_t height;

	int32_t x;
	int32_t y;
	int32_t z;

	bool show:1;

	void* mouse_ev_meta;
	void (*mouse_ev_handler)(void* meta, uint32_t x, uint32_t y, struct mouse_event* ev);

	// For debugging only
	const char* name;
};

struct window;
struct surface* surfaces;

static inline bool surface_intersect(struct surface* surface, size_t width, size_t height, size_t x, size_t y) {
	return (surface->x < (x + width) && (surface->x + surface->width) > x &&
		surface->y < (y + height) && (surface->y + surface->height) > y);
}

#define surface_blit_full(s) surface_blit(s, (s)->width, (s)->height, 0, 0)
void surface_blit(struct surface* surface, size_t width, size_t height, size_t x, size_t y);

struct surface* surface_new(cairo_surface_t* cs, size_t width, size_t height);
void surface_move(struct surface* surface, int32_t x, int32_t y);
void surface_add(struct surface* surface);
void render_init();
