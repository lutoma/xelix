#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <poll.h>
#include <png.h>

#include "util.h"
#include "window.h"
#include "mouse.h"
#include "bus.h"
#include "text.h"
#include "render.h"

int main(int argc, char* argv[]) {
	serial = fopen("/dev/serial1", "w");
	setvbuf(serial, NULL, _IONBF, 0);

	window_init();
	render_init();
	text_init();

	int gfxbus_fd = bus_init();
	int mouse_fd = mouse_init();

	int pid = fork();
	if(!pid) {
		execl("/usr/bin/gfxterm", "gfxterm");
		exit(-1);
	}

	struct pollfd pfds[2] = {
		{.fd = mouse_fd, .events = POLLIN},
		{.fd = gfxbus_fd, .events = POLLIN},
	};

	while(1) {
		pfds[0].revents = 0;
		pfds[1].revents = 0;
		if(poll(pfds, 2, -1) < 1) {
			continue;
		}

		if(pfds[0].revents & POLLIN) {
			handle_mouse();
		}

		if(pfds[1].revents & POLLIN) {
			bus_handle_msg();
		}

	}
}
