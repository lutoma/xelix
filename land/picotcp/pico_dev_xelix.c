#include "pico_device.h"
#include "pico_dev_xelix.h"
#include "pico_stack.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

struct pico_device_xelix {
	struct pico_device dev;
	int statistics_frames_out;
};

#define XELIX_MTU 1000
static int devno = 0;

static int pico_xelix_send(struct pico_device *dev, void *buf, int len) {
	struct pico_device_xelix* xdev = (struct pico_device_xelix*)dev;
	write(devno, buf, len);
	xdev->statistics_frames_out++;
	return len;
}

static int pico_xelix_poll(struct pico_device *dev, int loop_score) {
	/* We never have packet to receive, no score is used. */
	IGNORE_PARAMETER(dev);
	return loop_score;
}

struct pico_device* pico_xelix_create(const char* name) {
	struct pico_device_xelix* xdev = PICO_ZALLOC(sizeof(struct pico_device_xelix));

	if(!xdev) {
		return NULL;
	}

	devno = open("/dev/rtl1", O_RDWR);
	if(devno == -1) {
		perror("Could not open /dev/rtl1");
		return NULL;
	}

	dbg("pico_dev_xelix: Opened /dev/rtl1, devno %d\n", devno);

	if(pico_device_init((struct pico_device*)xdev, name, NULL)) {
		return NULL;
	}

	xdev->dev.overhead = 0;
	xdev->statistics_frames_out = 0;
	xdev->dev.send = pico_xelix_send;
	xdev->dev.poll = pico_xelix_poll;
	return (struct pico_device*)xdev;
}

static void __attribute__((destructor)) _close_xelix_dev(void) {
	if(devno) {
		close(devno);
	}
}
