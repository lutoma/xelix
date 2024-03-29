/* virtio_net.c: VirtIO network device
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef CONFIG_ENABLE_VIRTIO_NET

#include <net/net.h>
#include <bsp/virtio.h>
#include <net/virtio_net.h>
#include <bsp/i386-pci.h>
#include <log.h>
#include <portio.h>
#include <bitmap.h>
#include <int/int.h>
#include <mem/kmalloc.h>
#include <tasks/task.h>
#include <pico_device.h>

#define QUEUE_RX1 0
#define QUEUE_TX1 1

// Features
#define	VIRTIO_NET_F_CSUM	(1 << 0) /* Host handles pkts w/ partial csum */
#define	VIRTIO_NET_F_GUEST_CSUM	(1 << 1) /* Guest handles pkts w/ part csum */
#define	VIRTIO_NET_F_MAC	(1 << 5) /* Host has given MAC address. */
#define	VIRTIO_NET_F_GSO	(1 << 6) /* Host handles pkts w/ any GSO type */
#define	VIRTIO_NET_F_GUEST_TSO4	(1 << 7) /* Guest can handle TSOv4 in. */
#define	VIRTIO_NET_F_GUEST_TSO6	(1 << 8) /* Guest can handle TSOv6 in. */
#define	VIRTIO_NET_F_GUEST_ECN	(1 << 9) /* Guest can handle TSO[6] w/ ECN in */
#define	VIRTIO_NET_F_GUEST_UFO	(1 << 10) /* Guest can handle UFO in. */
#define	VIRTIO_NET_F_HOST_TSO4	(1 << 11) /* Host can handle TSOv4 in. */
#define	VIRTIO_NET_F_HOST_TSO6	(1 << 12) /* Host can handle TSOv6 in. */
#define	VIRTIO_NET_F_HOST_ECN	(1 << 13) /* Host can handle TSO[6] w/ ECN in */
#define	VIRTIO_NET_F_HOST_UFO	(1 << 14) /* Host can handle UFO in. */
#define	VIRTIO_NET_F_MRG_RXBUF	(1 << 15) /* Host can merge receive buffers. */
#define	VIRTIO_NET_F_STATUS	(1 << 16) /* Config.status available */
#define	VIRTIO_NET_F_CTRL_VQ	(1 << 17) /* Control channel available */
#define	VIRTIO_NET_F_CTRL_RX	(1 << 18) /* Control channel RX mode support */
#define	VIRTIO_NET_F_CTRL_VLAN	(1 << 19) /* Control channel VLAN filtering */
#define	VIRTIO_NET_F_CTRL_RX_EXTRA (1 << 20) /* Extra RX mode control support */

#define FEATURES_WANT VIRTIO_NET_F_MAC


static struct virtio_dev* dev = NULL;
// FIXME
static struct net_device* net_dev = NULL;

static uint32_t vendor_device_combos[][2] = {
	{0x1AF4, 0x1000}, {0x1AF4, 0x1041}, {(uint32_t)NULL}
};

struct virtio_net_hdr {
	uint8_t flags;
	uint8_t gso_type;
	uint16_t hdr_len;
	uint16_t gso_size;
	uint16_t csum_start;
	uint16_t csum_offset;
	//uint16_t num_buffers;
};

static char* feature_flags_verbose[] = {
	"Host CSUM",
	"Guest CSUM",
	"2", "3", "4",
	"MAC",
	"GSO",
	"TSOv4",
	"TSOv6",
	"TSO[6] + ECN",
	"UFO",
	"merge bfs",
	"Config.status",
	"ctrl chan",
	"ctrl chan RX",
	"VLAN",
	"RX mode control"
};

static int send(struct pico_device* pdev, void* data, int len) {
	if(!(dev->status & VIRTIO_PCI_STATUS_DRIVER_OK)) {
		return -1;
	}

	struct virtio_net_hdr* hdr = zmalloc(sizeof(struct virtio_net_hdr));
	void* buf = kmalloc(len);
	memcpy(buf, data, len);

	void* buffers[] = {hdr, buf};
	size_t lengths[] = {sizeof(struct virtio_net_hdr), len};

	if(virtio_write(dev, QUEUE_TX1, 2, buffers, lengths, NULL) < 0) {
		return -1;
	}

	return len;
}

static void used_cb(struct virtqueue* queue, struct virtq_desc* desc, uint32_t len) {
	if(queue->id == QUEUE_RX1) {
		if(net_dev) {
			net_receive(net_dev,
				(void*)(uint32_t)(desc->addr + sizeof(struct virtio_net_hdr)),
				len - sizeof(struct virtio_net_hdr));
		}
	}
}

static void int_handler(task_t* task, isf_t* state, int num) {
	inb(dev->pci_dev->iobase + 0x13);

	for(int i = 0; i < dev->num_queues; i++) {
		struct virtqueue* queue = &dev->queues[i];
		queue->available->flags = VIRTQ_AVAIL_F_NO_INTERRUPT;

		for(; queue->used_index < queue->used->idx; queue->used_index++) {
			struct virtq_used_elem* el = &queue->used->ring[queue->used_index % queue->size];
			struct virtq_desc* desc = &queue->descriptors[el->id];

			used_cb(queue, desc, el->len);

			if(desc->flags & VIRTQ_DESC_F_WRITE) {
				// Device write, reinsert desc into available
				virtio_write_avail(dev, queue, el->id);
			} else {
				// Driver write, clean up desc
				kfree((void*)(uint32_t)desc->addr);
				bzero(desc, sizeof(struct virtq_desc));
			}
		}

		queue->available->flags = 0;
	}
}

static int pci_cb(pci_device_t* pci_dev) {
	if(pci_check_vendor(pci_dev, vendor_device_combos) != 0) {
		return 1;
	}

	log(LOG_INFO, "virtio_net: Discovered device %p\n", pci_dev);

	dev = virtio_init_dev(pci_dev, FEATURES_WANT, 2);
	if(!dev) {
		return 1;
	}

	log(LOG_DEBUG, "virtio_net: Negotiated features (0x%x): ", dev->features);
	for(int i = 0; i < 16; i++) {
		if(bit_get(dev->features, i)) {
			// FIXME should use log
			serial_printf("%s ", feature_flags_verbose[i]);
		}
	}
	serial_printf("\n");

	dev->queues[QUEUE_TX1].available->flags = VIRTQ_AVAIL_F_NO_INTERRUPT;
	virtio_provide_descs(dev, QUEUE_RX1, 50, 1500);
	int_register(IRQ(dev->pci_dev->interrupt_line), int_handler, false);

	uint8_t mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
	if(dev->features & VIRTIO_NET_F_MAC) {
		for(int i = 0; i < 6; i++) {
			mac[i] = inb(dev->pci_dev->iobase + 0x14 + i);
		}
	}

	dev->status |= VIRTIO_PCI_STATUS_DRIVER_OK;
	virtio_write_status(dev);

	net_dev = net_add_device("vionet", mac, send);
	return 0;
}

void virtio_net_init(void) {
	pci_walk(pci_cb);
}

#endif /* ENABLE_VIRTIO_NET */
