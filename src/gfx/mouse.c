/* mouse.c: A generic PS2 mouse driver.
 *
 * Copyright © 2020 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <int/int.h>
#include <fs/sysfs.h>
#include <fs/poll.h>
#include <portio.h>
#include <errno.h>
#include <bitmap.h>
#include <buffer.h>

struct mouse_event {
	int16_t x;
	int16_t y;
	uint8_t button_left:1;
	uint8_t button_right:1;
	uint8_t button_middle:1;
};

static struct buffer* buf = NULL;

static inline void intr_handler() {
	static uint8_t mouse_cycle = 0;
	static struct mouse_event ev;
	static uint8_t status;

	switch(mouse_cycle) {
		case 0:
			status = inb(0x60);
			ev.button_left = bit_get(status, 0);
			ev.button_right = bit_get(status, 1);
			ev.button_middle = bit_get(status, 2);
			break;
		case 1:
			ev.x = inb(0x60) - ((status << 4) & 0x100);
			break;
		case 2:
			ev.y = (inb(0x60) - ((status << 3) & 0x100)) * -1;
			buffer_write(buf, &ev, sizeof(ev));
			break;
	}

	mouse_cycle++;
	mouse_cycle %= 3;
}

static inline void mouse_wait(uint8_t a_type) {
	uint32_t _time_out=100000;
	if(a_type==0) {
		while(_time_out--) {
			if((inb(0x64) & 1)==1) {
				return;
			}
		}
		return;
	} else {
		while(_time_out--) {
			if((inb(0x64) & 2)==0) {
				return;
			}
		}
		return;
	}
}

static inline void mouse_write(uint8_t cmd) {
	mouse_wait(1);
	//Tell the mouse we are sending a command
	outb(0x64, 0xD4);
	mouse_wait(1);
	outb(0x60, cmd);
}

static inline uint8_t mouse_read() {
	mouse_wait(0);
	return inb(0x60);
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(!buffer_size(buf) && ctx->fp->flags & O_NONBLOCK) {
		sc_errno = EAGAIN;
		return -1;
	}

	while(!buffer_size(buf)) {
		scheduler_yield();
	}

	return buffer_pop(buf, dest, size);
}

static int sfs_poll(struct vfs_callback_ctx* ctx, int events) {
	if(events & POLLIN && buffer_size(buf)) {
		return POLLIN;
	}
	return 0;
}

void gfx_mouse_init() {
	buf = buffer_new(10);
	if(!buf) {
		return;
	}

	// Enable auxiliary mouse device
	mouse_wait(1);
	outb(0x64, 0xA8);

	// Enable interrupts
	mouse_wait(1);
	outb(0x64, 0x20);
	mouse_wait(0);
	uint8_t status = (inb(0x60) | 2);
	mouse_wait(1);
	outb(0x64, 0x60);
	mouse_wait(1);
	outb(0x60, status);

	// Use default settings
	mouse_write(0xF6);
	mouse_read();

	// Enable mouse
	mouse_write(0xF4);
	mouse_read();

	int_register(IRQ(12), &intr_handler, false);

	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
		.poll = sfs_poll,
	};
	sysfs_add_dev("mouse1", &sfs_cb);
}
