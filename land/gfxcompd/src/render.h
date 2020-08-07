#pragma once

#include <stdint.h>
#include <stdbool.h>

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


struct gfx_ul_blit_cmd {
	unsigned int handle_id;
	size_t x;
	size_t y;
	size_t width;
	size_t height;
};

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
};

struct window;

#define surface_blit_full(s) surface_blit(s, (s)->width, (s)->height, 0, 0)
void surface_blit(struct surface* surface, size_t width, size_t height, size_t x, size_t y);

struct surface* surface_new(size_t width, size_t height);
void surface_add(struct surface* surface);
void render_init();
