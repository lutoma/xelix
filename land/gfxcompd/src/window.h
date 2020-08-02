#pragma once

#include <stdint.h>
#include <cairo/cairo.h>

struct window {
	uint32_t id;
	struct window* next;
	uint32_t* buffer;
	char title[1024];

	// Window size _excluding_ decoration
	size_t width;
	size_t height;

	int32_t x;
	int32_t y;
	int32_t z;
	cairo_surface_t* surface;

	int32_t dx;
	int32_t dy;
	cairo_surface_t* decoration;
};

struct window* windows;

struct window* window_new(const char* title, size_t width, size_t height, uint32_t* data);
struct window* window_get(uint32_t id);
void window_update(struct window* win);
void window_set_position(struct window* win, int32_t x, int32_t y);
void window_add(struct window* window);
void window_init();
