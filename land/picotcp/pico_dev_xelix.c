#include "pico_device.h"
#include "pico_dev_xelix.h"
#include "pico_stack.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

struct pico_device_xelix {
	struct pico_device dev;
	void* rbuf;
	int statistics_frames_out;
	int devno;
};

#define XELIX_MTU 1000

static int pico_xelix_send(struct pico_device *dev, void *buf, int len) {
	struct pico_device_xelix* xdev = (struct pico_device_xelix*)dev;
	write(xdev->devno, buf, len);
	xdev->statistics_frames_out++;
	return len;
}

static int pico_xelix_poll(struct pico_device *dev, int loop_score) {
	struct pico_device_xelix* xdev = (struct pico_device_xelix*)dev;

	int len = read(xdev->devno, xdev->rbuf, 0x1000);
	if(len > 0) {
		pico_stack_recv(xdev, xdev->rbuf, len);
		loop_score--;
	}

	/* return (original_loop_score - amount_of_packets_received) */
	return loop_score;
}

void pico_xelix_destroy(struct pico_device* dev) {
	struct pico_device_xelix* xdev = (struct pico_device_xelix*)dev;
	close(xdev->devno);
}

struct pico_device* pico_xelix_create(const char* name) {
	struct pico_device_xelix* xdev = PICO_ZALLOC(sizeof(struct pico_device_xelix));
	struct pico_ethdev* eth = PICO_ZALLOC(sizeof(struct pico_ethdev));

	if(!xdev || !eth) {
		return NULL;
	}

	xdev->devno = open("/dev/ne2k1", O_RDWR);
	if(xdev->devno == -1) {
		perror("Could not open /dev/ne2k1");
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
