#include <stdio.h>
#include <poll.h>

#include "util.h"
#include "window.h"
#include "mouse.h"
#include "bus.h"
#include "text.h"
#include "render.h"
#include "bar.h"

int main(int argc, char* argv[]) {
	serial = fopen("/dev/serial1", "w");
	setvbuf(serial, NULL, _IONBF, 0);

	text_init();
	window_init();
	render_init();
	bar_init();

	int gfxbus_fd = bus_init();
	int mouse_fd = mouse_init();

	struct pollfd pfds[2] = {
		{.fd = mouse_fd, .events = POLLIN},
		{.fd = gfxbus_fd, .events = POLLIN},
	};

	while(1) {
		pfds[0].revents = 0;
		pfds[1].revents = 0;
		if(poll(pfds, 2, 1) < 1) {
			continue;
		}

		if(pfds[0].revents & POLLIN) {
			handle_mouse();
		}

		if(pfds[1].revents & POLLIN) {
			bus_handle_msg();
		}

		update_bar();
	}
}
