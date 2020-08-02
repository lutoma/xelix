#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "window.h"

struct {
	int fd;
	uint32_t* buf;

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
} fb;

void render();
void win_blit(struct window* win);
void render_init();
