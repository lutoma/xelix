#pragma once

#include <stdint.h>
#include <cairo/cairo.h>
#include "render.h"

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

	struct surface* surface;
	struct surface* decoration;
};

struct window* windows;
struct window* active_window;

struct window* window_new(uint32_t wid, const char* title, size_t width, size_t height, uint32_t* data);
struct window* window_get(uint32_t id);
void window_toggle_show(struct window* win, int show);
void window_blit(struct window* win, size_t width, size_t height, size_t x, size_t y);
void window_set_position(struct window* win, int32_t x, int32_t y);
void window_add(struct window* window);
void window_init();
