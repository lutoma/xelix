#pragma once

#include <stdint.h>
#include <cairo/cairo.h>

struct mouse_event {
	int16_t x;
	int16_t y;
	uint8_t button_left:1;
	uint8_t button_right:1;
	uint8_t button_middle:1;
};

void handle_mouse();
void mouse_render_cursor(cairo_t* cr);
int mouse_init();
