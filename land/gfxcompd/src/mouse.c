#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <cairo/cairo.h>

#include "util.h"
#include "render.h"
#include "mouse.h"
#include "window.h"

static int mouse_fd = -1;
static struct surface* surface = NULL;
static uint32_t pos_x;
static uint32_t pos_y;

void handle_mouse() {
	struct mouse_event ev;
	size_t sz = sizeof(struct mouse_event);

	if(read(mouse_fd, &ev, sz) < sz) {
		return;
	}

	if(ev.x < 0 && pos_x < ev.x * -1) {
		pos_x = 0;
	} else if(pos_x + ev.x > gfx_handle.width) {
		pos_x = gfx_handle.width;
	} else {
		pos_x += ev.x;
	}

	if(ev.y < 0 && pos_y < ev.y * -1) {
		pos_y = 0;
	} else if(pos_y + ev.y > gfx_handle.height) {
		pos_y = gfx_handle.height;
	} else {
		pos_y += ev.y;
	}

	// Move cursor surface
	surface_move(surface, (int32_t)pos_x, (int32_t)pos_y);

	// Run event callbacks
	struct surface* top = NULL;
	int32_t top_z = -100000;

	struct surface* cbs = surfaces;
	for(; cbs; cbs = cbs->next) {
		if(cbs->mouse_ev_handler && cbs->z > top_z && surface_intersect(cbs, 1, 1, pos_x, pos_y)) {
			top_z = cbs->z;
			top = cbs;
		}
	}

	if(top) {
		top->mouse_ev_handler(top->mouse_ev_meta, pos_x, pos_y, &ev);
	}
}

int mouse_init() {
	mouse_fd = open("/dev/mouse1", O_RDONLY);
	if(mouse_fd == -1) {
		perror("Could not open /dev/mouse1");
		return -1;
	}

	cairo_surface_t* cs = cairo_image_surface_create_from_png("/usr/share/gfxcompd/cursor.png");
	if(!cs) {
		perror("Could not open cursor image");
		return -1;
	}

	surface = surface_new(cs, cairo_image_surface_get_width(cs), cairo_image_surface_get_height(cs));
	pos_x = gfx_handle.width / 2 + cairo_image_surface_get_width(cs) / 2;
	pos_y = gfx_handle.height / 2 + cairo_image_surface_get_height(cs) / 2;

	surface_move(surface, (int32_t)pos_x, (int32_t)pos_y);
	surface->z = 1000;
	surface_add(surface);
	return mouse_fd;
}
