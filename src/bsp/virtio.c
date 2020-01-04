/* virtio.c: VirtIO support functions
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

#include <bsp/virtio.h>
#include <bsp/i386-pci.h>
#include <mem/kmalloc.h>
#include <portio.h>

#define ioutw(pin, val) outw(dev->pci_dev->iobase + (pin), val)
#define ioutl(pin, val) outl(dev->pci_dev->iobase + (pin), val)
#define iinb(pin) inb(dev->pci_dev->iobase + (pin))
#define iinw(pin) inw(dev->pci_dev->iobase + (pin))
#define iinl(pin) inl(dev->pci_dev->iobase + (pin))

static inline int negotiate_features(struct virtio_dev* dev, uint32_t want_cap) {
	// Get device supported features, pick out the ones we want and confirm
	uint32_t dev_features = iinl(VIRTIO_IO_DEV_FEATURE);
	dev->features = dev_features & want_cap;
	ioutl(VIRTIO_IO_DRV_FEATURE, dev->features);
	dev->status |= VIRTIO_PCI_STATUS_FEATURES_OK;
	virtio_write_status(dev);

	// Check if device is happy
	sleep_ticks(10);
	if(!(iinb(VIRTIO_IO_STATUS) & VIRTIO_PCI_STATUS_FEATURES_OK)) {
		return -1;
	}

	return 0;
}

static inline int write_desc_chain(struct virtqueue* queue, int num_buffers,
	void** buffers, size_t* lengths, int* flags) {

	size_t desc_head = queue->desc_index;

	for(int i = 0; i < num_buffers; i++) {
		struct virtq_desc* desc = &queue->descriptors[queue->desc_index];
		queue->desc_index = (queue->desc_index + 1) % queue->size;

		desc->len = lengths[i];
		desc->addr = (uint64_t)(uintptr_t)buffers[i];
		desc->flags = 0;
		desc->next = 0;

		if(flags) {
			desc->flags |= flags[i];
		}

		if(i < num_buffers - 1) {
			desc->flags |= VIRTQ_DESC_F_NEXT;
			desc->next = queue->desc_index;
		}
	}

	return desc_head;
}

int virtio_write(struct virtio_dev* dev, uint8_t queue_id, int num_buffers,
	void** buffers, size_t* lengths, int* flags) {

	struct virtqueue* queue = &dev->queues[queue_id];
	size_t desc_head = write_desc_chain(queue, num_buffers, buffers, lengths, flags);
	virtio_write_avail(dev, queue, desc_head);
	return desc_head;
}

void virtio_provide_descs(struct virtio_dev* dev, uint8_t queue_id, int num, size_t size) {
	struct virtqueue* queue = &dev->queues[queue_id];

	for(int i = 0; i < num; i++) {
		void* buf = kmalloc(size);

		int flags[] = {VIRTQ_DESC_F_WRITE};
		int desc = write_desc_chain(queue, 1, &buf, &size, flags);

		size_t av_index = (queue->available->idx + i) % queue->size;
		queue->available->ring[av_index] = desc;
	}

	__sync_synchronize();
	queue->available->idx += num;
	ioutw(0x10, queue_id);
}

static inline int setup_virtqueue(struct virtio_dev* dev, uint8_t queue_id) {
	ioutw(VIRTIO_IO_QUEUE_SELECT, queue_id);
	struct virtqueue* queue = &dev->queues[queue_id];
	queue->id = queue_id;

	// Check if this vq is actually supported by the card
	queue->size = iinw(VIRTIO_IO_QUEUE_SIZE);
	if(queue->size < 1) {
		return -1;
	}

	size_t desc_size = ALIGN(queue->size * 16, 4);

	// This seems to contradict the standard, but it's the alignment QEMU wants
	size_t available_size = ALIGN((queue->size * 2) + 6, PAGE_SIZE);
	size_t used_size = (queue->size * 8) + 6;

	void* buf = zmalloc_a(desc_size + available_size + used_size);
	queue->descriptors = buf;
	queue->available = buf + desc_size;
	queue->used = buf + desc_size + available_size;

	__sync_synchronize();
	ioutl(VIRTIO_IO_QUEUE_PFN, (uintptr_t)buf >> 12);
	return 0;
}

struct virtio_dev* virtio_init_dev(pci_device_t* pci_dev, uint32_t cap, int queues) {
	struct virtio_dev* dev = zmalloc(sizeof(struct virtio_dev) + sizeof(struct virtqueue) * queues);
	dev->pci_dev = pci_dev;
	dev->num_queues = queues;

	dev->status = VIRTIO_PCI_STATUS_RESET;
	virtio_write_status(dev);

	if(iinb(VIRTIO_IO_STATUS) != 0) {
		log(LOG_ERR, "virtio: Device reset failed.\n");
		kfree(dev);
		return NULL;
	}

	dev->status = VIRTIO_PCI_STATUS_ACKNOWLEDGE | VIRTIO_PCI_STATUS_DRIVER;
	virtio_write_status(dev);

	if(negotiate_features(dev, cap) < 0) {
		log(LOG_ERR, "virtio: Feature negotiation failed\n");
		dev->status = VIRTIO_PCI_STATUS_FAILED;
		virtio_write_status(dev);
		kfree(dev);
		return NULL;
	}

	for(int i = 0; i < queues; i++) {
		if(setup_virtqueue(dev, i) < 0) {
			log(LOG_ERR, "virtio: Could not set up queue %d\n", i);
			dev->status = VIRTIO_PCI_STATUS_FAILED;
			virtio_write_status(dev);
			kfree(dev);
			return NULL;
		}
	}

	return dev;
}
