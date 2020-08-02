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
static cairo_surface_t* surface = NULL;
static uint32_t pos_x;
static uint32_t pos_y;

int handle_mouse() {
	struct mouse_event ev;
	size_t sz = sizeof(struct mouse_event);

	if(read(mouse_fd, &ev, sz) < sz) {
		return 0;
	}

	if(ev.x < 0 && pos_x < ev.x * -1) {
		pos_x = 0;
	} else if(pos_x + ev.x > fb.width) {
		pos_x = fb.width;
	} else {
		pos_x += ev.x;
	}

	if(ev.y < 0 && pos_y < ev.y * -1) {
		pos_y = 0;
	} else if(pos_y + ev.y > fb.height) {
		pos_y = fb.height;
	} else {
		pos_y += ev.y;
	}


	if(ev.button_left) {
		fprintf(serial, "x: %3d, y: %3d, left: %d right: %d middle: %d\n",
			ev.x, ev.y, ev.button_left, ev.button_right, ev.button_middle);


		//if(main_win) {
		//	window_set_position(main_win, pos_x, pos_y);
		//}
	//	msg_layer->x += ev.x;
	//	msg_layer->y += ev.y;
	}

	return 1;
}

void mouse_render_cursor(cairo_t* cr) {
	if(surface) {
		cairo_set_source_surface(cr, surface, pos_x, pos_y);
		cairo_paint(cr);
	}
}

int mouse_init() {
	mouse_fd = open("/dev/mouse1", O_RDONLY);
	if(mouse_fd == -1) {
		perror("Could not open /dev/mouse1");
		return -1;
	}

	surface = cairo_image_surface_create_from_png("/usr/share/gfxcompd/cursor.png");
	pos_x = fb.width / 2 + cairo_image_surface_get_width(surface) / 2;
	pos_x = fb.height / 2 + cairo_image_surface_get_height(surface) / 2;
	return mouse_fd;
}
