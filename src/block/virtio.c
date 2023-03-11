/* virtio_block.c: VirtIO block storage device
 * Copyright © 2019-2023 Lukas Martini
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

#ifdef CONFIG_ENABLE_VIRTIO_BLOCK

#include <net/net.h>
#include <bsp/virtio.h>
#include <bsp/i386-pci.h>
#include <log.h>
#include <portio.h>
#include <fs/vfs.h>
#include <block/block.h>
#include <int/int.h>
#include <mem/vm.h>
#include <mem/kmalloc.h>
#include <tasks/task.h>

#define VIRTIO_BLK_F_SIZE_MAX (1 << 1)
#define VIRTIO_BLK_F_SEG_MAX (1 << 2)
#define VIRTIO_BLK_F_GEOMETRY (1 << 4)
#define VIRTIO_BLK_F_RO (1 << 5)
#define VIRTIO_BLK_F_BLK_SIZE (1 << 6)
#define VIRTIO_BLK_F_FLUSH (1 << 9)
#define VIRTIO_BLK_F_TOPOLOGY (1 << 10)
#define VIRTIO_BLK_F_CONFIG_WCE (1 << 11)

#define VIRTIO_BLK_T_IN  0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_FLUSH 4

#define VIRTIO_BLK_S_OK 0
#define VIRTIO_BLK_S_IOERR 1
#define VIRTIO_BLK_S_UNSUPP 2

struct virtio_blk_req {
	uint32_t type;
	uint32_t reserved;
	uint64_t sector;
};

static struct virtio_dev* dev = NULL;
static uint32_t vendor_device_combos[][2] = {
	{0x1AF4, 0x1001}, {0x1AF4, 0x1042}, {(uint32_t)NULL}
};

static void int_handler(task_t* task, isf_t* state, int num) {
	inb(dev->pci_dev->iobase + 0x13);
}

static uint64_t send_request(struct virtio_dev* dev, int type, uint64_t lba, uint64_t num_blocks, void* buf) {
	if(num_blocks < 1) {
		return -1;
	}

	if(!(dev->status & VIRTIO_PCI_STATUS_DRIVER_OK)) {
		return -1;
	}

	struct virtio_blk_req hdr = {
		.type = type,
		.reserved = 0,
		.sector = lba
	};

	volatile uint8_t status = 0xff;
	void* buffers[] = {
		valloc_translate(VM_KERNEL, &hdr, false),
		valloc_translate(VM_KERNEL, buf, false),
		valloc_translate(VM_KERNEL, (void*)&status, false),
	};

	if(!(buffers[0] && buffers[1] && buffers[2])) {
		log(LOG_ERR, "virtio_block: Could not map buffers to phys mem in send_request\n");
		return -1;
	}

	size_t lengths[] = {sizeof(struct virtio_blk_req), num_blocks * 512, sizeof(uint8_t)};

	int user_buffer_flag = (type == VIRTIO_BLK_T_IN) ? VIRTQ_DESC_F_WRITE : 0;
	int flags[] = {0, user_buffer_flag, VIRTQ_DESC_F_WRITE};

	if(virtio_write(dev, 0, 3, buffers, lengths, flags) < 0) {
		return -1;
	}

	while(status == 0xff) {
		scheduler_yield();
	}

	if(status != VIRTIO_BLK_S_OK) {
		log(LOG_ERR, "virtio_block: Request type %d, lba %d failed (align %d)\n", type, lba, (uintptr_t)buf % 0x1000);
		return -1;
	}

	return num_blocks;
}

static uint64_t read_cb(struct vfs_block_dev* block_dev, uint64_t lba, uint64_t num_blocks, void* buf) {
	if(num_blocks < 16) {
		return send_request(dev, VIRTIO_BLK_T_IN, lba, num_blocks, buf);
	}

	for(uint64_t i = 0; i < num_blocks; i++) {
		if(send_request(dev, VIRTIO_BLK_T_IN, lba + i, 1, buf + (uint32_t)i * 512) == -1) {
			return i;
		}
	}

	return num_blocks;
}

static uint64_t write_cb(struct vfs_block_dev* block_dev, uint64_t lba, uint64_t num_blocks, void* buf) {
	if(dev->features & VIRTIO_BLK_F_RO) {
		return -1;
	}

	//log(LOG_DEBUG, "virtio write_cb lba %d buf %#x\n", lba, buf);
	return send_request(dev, VIRTIO_BLK_T_OUT, lba, num_blocks, buf);
}

void virtio_block_init() {
	pci_device_t** devices = (pci_device_t**)kmalloc(sizeof(void*));
	uint32_t ndevices = pci_search(devices, vendor_device_combos, 1);
	log(LOG_INFO, "virtio_block: Discovered %d devices.\n", ndevices);

	if(ndevices) {
		dev = virtio_init_dev(devices[0], 0, 1);
		if(!dev) {
			return;
		}

		if(dev->features & VIRTIO_BLK_F_RO) {
			log(LOG_INFO, "virtio_block: Device is read-only\n");
		}

		dev->queues[0].available->flags = VIRTQ_AVAIL_F_NO_INTERRUPT;
		dev->queues[0].used->flags = VIRTQ_USED_F_NO_NOTIFY;
		int_register(IRQ(dev->pci_dev->interrupt_line), int_handler, false);

		dev->status |= VIRTIO_PCI_STATUS_DRIVER_OK;
		virtio_write_status(dev);

		vfs_block_register_dev("vioblk1", (uint64_t)0, read_cb, write_cb, NULL);
	}
}

#endif /* ENABLE_VIRTIO_BLOCK */
