#pragma once

#include <stdint.h>
#include "window.h"

struct {
	int fd;
	uint32_t* addr;
	uint32_t* buf;
	uint32_t bpp;
	size_t width;
	size_t height;
	size_t pitch;
	size_t size;
} fb;

void render();
void win_blit(struct window* win);
void render_init();
