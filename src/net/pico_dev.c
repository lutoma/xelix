/* pico_dev.c: PicoTCP device integration
 * Copyright © 2019 Lukas Martini
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

#include "pico_device.h"
#include "pico_stack.h"
#include <net/pico_dev.h>
#include <fs/vfs.h>

struct pico_device_xelix {
	struct pico_device dev;
	void* rbuf;
	int statistics_frames_out;
	vfs_file_t* fp;
};

#define XELIX_MTU 1000

static int pico_xelix_send(struct pico_device *dev, void *buf, int len) {
	struct pico_device_xelix* xdev = (struct pico_device_xelix*)dev;
	vfs_write(buf, len, xdev->fp);
	xdev->statistics_frames_out++;
	return len;
}

static int pico_xelix_poll(struct pico_device *dev, int loop_score) {
	struct pico_device_xelix* xdev = (struct pico_device_xelix*)dev;

	int len = vfs_read(xdev->rbuf, 0x1000, xdev->fp);
	if(len > 0) {
		pico_stack_recv(xdev, xdev->rbuf, len);
		loop_score--;
	}

	/* return (original_loop_score - amount_of_packets_received) */
	return loop_score;
}

void pico_xelix_destroy(struct pico_device* dev) {
	struct pico_device_xelix* xdev = (struct pico_device_xelix*)dev;
	vfs_close(xdev->fp);
}

struct pico_device* pico_xelix_create(const char* name) {
	struct pico_device_xelix* xdev = PICO_ZALLOC(sizeof(struct pico_device_xelix));
	struct pico_ethdev* eth = PICO_ZALLOC(sizeof(struct pico_ethdev));

	if(!xdev || !eth) {
		return NULL;
	}

	xdev->fp = vfs_open("/dev/ne2k1", O_RDWR, NULL);
	if(!xdev->fp) {
		printf("Could not open /dev/ne2k1");
		return NULL;
	}

	if(pico_device_init((struct pico_device*)xdev, name, NULL)) {
		return NULL;
	}

	xdev->rbuf = PICO_ZALLOC(0x1000);

	eth->mac.addr[0] = 0x52;
	eth->mac.addr[1] = 0x54;
	eth->mac.addr[2] = 0x00;
	eth->mac.addr[3] = 0x12;
	eth->mac.addr[4] = 0x34;
	eth->mac.addr[5] = 0x56;
	xdev->dev.eth = eth;
	xdev->dev.overhead = 0;
	xdev->statistics_frames_out = 0;
	xdev->dev.send = pico_xelix_send;
	xdev->dev.poll = pico_xelix_poll;
	xdev->dev.destroy = pico_xelix_destroy;
	return (struct pico_device*)xdev;
}
