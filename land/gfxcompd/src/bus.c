#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "util.h"
#include "bus.h"
#include "window.h"
#include "window.h"

struct msg_window_new {
	void* addr;
	char title[1024];
	size_t width;
	size_t height;
	int32_t x;
	int32_t y;
};

struct msg_blit {
	uint32_t wid;
	size_t width;
	size_t height;
	int32_t x;
	int32_t y;
};

static int gfxbus_fd = -1;

size_t msg_sizes[] = {
	0,
	sizeof(struct msg_window_new),
	sizeof(struct msg_blit),
};

int bus_handle_msg() {
	uint16_t msg_type = -1;
	size_t rd = read(gfxbus_fd, &msg_type, 2);
	if(rd < 2) {
		return 0;
	}

	if(msg_type < 1 || msg_type > 2) {
		fprintf(serial, "Unknown message type: %u.\n", msg_type);
		return 0;
	}

	size_t msg_size = msg_sizes[msg_type];
	void* buf = NULL;

	if(msg_size) {
		buf = malloc(msg_size);
		if(read(gfxbus_fd, buf, msg_size) != msg_size) {
			fprintf(serial, "Could not read message: %s\n", strerror(errno));
			return 0;
		}
	}

	// New window
	if(msg_type == 1) {
		struct msg_window_new* msg = (struct msg_window_new*)buf;
		struct window* win = window_new(msg->title, msg->width, msg->height, msg->addr);
		window_set_position(win, msg->x, msg->y);
		window_add(win);
		return 0;
	}

	// Blit
	if(msg_type == 2) {
		struct msg_blit* msg = (struct msg_blit*)buf;
		struct window* win = window_get(msg->wid);
		if(!win) {
			return 0;
		}

		window_blit(win, msg->width, msg->height, msg->x, msg->y);
		return 0;
	}

	return 0;
}

int bus_init() {
	// Get gfxbus handle
	gfxbus_fd = open("/dev/gfxbus", O_RDWR);
	if(!gfxbus_fd) {
		perror("Could not open /dev/gfxbus");
		exit(EXIT_FAILURE);
	}

	// Register as gfxbus master
	ioctl(gfxbus_fd, 0x2f01, (uint32_t)0);
	return gfxbus_fd;
}
