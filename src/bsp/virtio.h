#pragma once

/* Copyright © 2019 Lukas Martini
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

#include <bsp/i386-pci.h>
#include <portio.h>

/* Virtio product id (subsystem) */
#define PCI_PRODUCT_VIRTIO_NETWORK  1
#define PCI_PRODUCT_VIRTIO_BLOCK    2
#define PCI_PRODUCT_VIRTIO_CONSOLE  3
#define PCI_PRODUCT_VIRTIO_ENTROPY  4
#define PCI_PRODUCT_VIRTIO_BALLOON  5
#define PCI_PRODUCT_VIRTIO_9P       9

#define VIRTIO_IO_DEV_FEATURE 0
#define VIRTIO_IO_DRV_FEATURE 4
#define VIRTIO_IO_STATUS 18
#define VIRTIO_IO_QUEUE_SELECT 14
#define VIRTIO_IO_QUEUE_SIZE 12
#define VIRTIO_IO_QUEUE_PFN 8

#define VIRTIO_PCI_STATUS_RESET 0x00
#define VIRTIO_PCI_STATUS_ACKNOWLEDGE 0x01
#define VIRTIO_PCI_STATUS_DRIVER 0x02
#define VIRTIO_PCI_STATUS_DRIVER_OK 0x04
#define VIRTIO_PCI_STATUS_FEATURES_OK 0x8
#define VIRTIO_PCI_STATUS_FAILED 0x80

// This marks a buffer as continuing via the next field.
#define VIRTQ_DESC_F_NEXT 1
// This marks a buffer as device write-only (otherwise device read-only).
#define VIRTQ_DESC_F_WRITE 2
// This means the buffer contains a list of buffer descriptors.
#define VIRTQ_DESC_F_INDIRECT 4

struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;

    // Next field if flags & NEXT
    uint16_t next;
};


#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
struct virtq_avail {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[];
};

#define VIRTQ_USED_F_NO_NOTIFY 1
struct virtq_used_elem {
	/* Index of start of used descriptor chain. */
	uint32_t id;
	/* Total length of the descriptor chain which was used (written to) */
	uint32_t len;
};

struct virtq_used {
	uint16_t flags;
	uint16_t idx;
	struct virtq_used_elem ring[];
};

struct virtqueue {
	struct virtq_desc* descriptors;
	struct virtq_avail* available;
	struct virtq_used* used;
	int id;
	size_t size;
	size_t desc_index;
	size_t used_index;
};

struct virtio_dev {
	pci_device_t* pci_dev;
	uint32_t features;
	uint32_t status;
	size_t num_queues;
	struct virtqueue queues[];
};

static inline void virtio_write_status(struct virtio_dev* dev) {
	outb(dev->pci_dev->iobase + VIRTIO_IO_STATUS, dev->status);
}

static inline void virtio_write_avail(struct virtio_dev* dev, struct virtqueue* queue, int desc_no) {
	queue->available->ring[queue->available->idx % queue->size] = desc_no;
	__sync_synchronize();
	queue->available->idx++;
	outw(dev->pci_dev->iobase + 0x10, queue->id);
}

int virtio_write(struct virtio_dev* dev, uint8_t queue_id, int num_buffers,
	void** buffers, size_t* lengths, int* flags);

void virtio_provide_descs(struct virtio_dev* dev, uint8_t queue_id, int num, size_t size);
struct virtio_dev* virtio_init_dev(pci_device_t* dev, uint32_t cap, int queues);
