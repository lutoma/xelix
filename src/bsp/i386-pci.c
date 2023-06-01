/* pci.c: PCI support functions
 * Copyright © 2011-2023 Lukas Martini
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
#include <mem/kmalloc.h>
#include <fs/sysfs.h>
#include <string.h>
#include <panic.h>
#include <portio.h>
#include <log.h>

#define PORT_CONFIG_ADDR  0x0CF8
#define PORT_CONFIG_DATA  0x0CFC

#define CONFIG_HEADER_DEVICE   2
#define CONFIG_HEADER_REVISION 8
#define CONFIG_HEADER_PROG_IF  9
#define CONFIG_HEADER_SUBCLASS 10
#define CONFIG_HEADER_CLASS    11
#define CONFIG_HEADER_TYPE     15
#define CONFIG_HEADER_INT_LINE 0x3c
#define CONFIG_HEADER_INT_PIN  0x3d

#define get_address(bus, dev, func, offset) (0x80000000 | (bus << 16) | \
	(dev << 11) | (func << 8) | (offset & 0xFC))


static pci_device_t* pci_devices = NULL;

uint32_t pci_config_read(pci_device_t* dev,	uint8_t offset, int size) {
	uint32_t addr = get_address(dev->bus, dev->dev, dev->func, offset);
	outl(PORT_CONFIG_ADDR, addr);

	switch(size) {
		case 4:
			return inl(PORT_CONFIG_DATA);
		case 2:
			return inw(PORT_CONFIG_DATA + (offset & 2));
		case 1:
			return inb(PORT_CONFIG_DATA + (offset & 3));
		default:
			return -1;
	}
}

void pci_config_write(pci_device_t* dev, uint8_t offset, uint32_t val) {
	uint32_t addr = get_address(dev->bus, dev->dev, dev->func, offset);
	outl(PORT_CONFIG_ADDR, addr);
	outl(PORT_CONFIG_DATA, val);
}

uint32_t pci_get_bar(pci_device_t* device, uint8_t bar) {
	assert(bar <= 5);

	if(unlikely(device->header_type == 0x2 || (device->header_type == 0x1 && bar < 2))) {
		return 0;
	}

	uint8_t _register = 0x10 + 0x4 * bar;
	return pci_config_read(device, _register, 4);
}

static inline void try_load_device(uint8_t bus, uint8_t dev, uint8_t func) {
	outl(PORT_CONFIG_ADDR, get_address(bus, dev, func, 0));
	uint16_t vendor = inw(PORT_CONFIG_DATA);

	/* Non-existant pdevs should have all config bits pulled high, but on
	 * some chipsets (emulators?) they were zero.
	 */
	if(vendor == 0xffff || vendor == 0) {
		return;
	}

	pci_device_t* pdev = kmalloc(sizeof(pci_device_t));

	pdev->bus = bus;
	pdev->dev = dev;
	pdev->func = func;
	pdev->vendor = vendor;
	pdev->header_type = pci_config_read(pdev, CONFIG_HEADER_TYPE, 1);
	pdev->device = pci_config_read(pdev, CONFIG_HEADER_DEVICE, 2);
	pdev->revision = pci_config_read(pdev, CONFIG_HEADER_REVISION, 1);
	pdev->prog_if = pci_config_read(pdev, CONFIG_HEADER_PROG_IF, 1);
	pdev->subclass = pci_config_read(pdev, CONFIG_HEADER_SUBCLASS, 1);
	pdev->class = pci_config_read(pdev, CONFIG_HEADER_CLASS, 1);

	if(pdev->header_type == 0) {
		pdev->interrupt_line = pci_config_read(pdev, CONFIG_HEADER_INT_LINE, 1);
		pdev->interrupt_pin = pci_config_read(pdev, CONFIG_HEADER_INT_PIN, 1);

		for(int i = 0; i < 6; i++) {
			uint32_t bar = pci_get_bar(pdev, i);
			if(bar & 0x1 && !pdev->iobase) {
				pdev->iobase = bar & 0xfffffff0;
			} else if(!(bar & 0x1) && !pdev->membase) {
				pdev->membase = bar & 0xfffffff0;
			}
		}
	}

	pdev->next = pci_devices;
	pci_devices = pdev;

	log(LOG_INFO, "  %02d:%02d.%d: %04x:%04x rev %-2x class %04x iobase %-4x type %-2x int %-2d pin %d\n",
			pdev->bus, pdev->dev, pdev->func, pdev->vendor,
			pdev->device, pdev->revision, pdev->class,
			pdev->iobase, pdev->header_type, pdev->interrupt_line,
			pdev->interrupt_pin);
}

int pci_walk(int (*callback)(pci_device_t* dev)) {
	for(pci_device_t* dev = pci_devices; dev; dev = dev->next) {
		int ret = callback(dev);
		if(ret < 1) {
			return ret;
		}
	}

	return 0;
}

/* Checks a PCI device against an array of vendor + device ID combos.
 *
 * static uint32_t vendor_device_combos[][2] = {
 * 	{vendor_id, device_id},
 * 	{vendor_id, device_id},
 * 	{(uint32_t)NULL}
 * };
 */
int pci_check_vendor(pci_device_t* dev, const uint32_t combos[][2]) {
	for(int i = 0; combos[i][0]; i++) {
		if(dev->vendor == combos[i][0] && dev->device == combos[i][1]) {
			return 0;
		}
	}

	return -1;
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	size_t rsize = 0;
	pci_device_t* dev = pci_devices;
	for(; dev; dev = dev->next) {
		uint16_t combined_class = dev->class << 8 | dev->subclass;
		sysfs_printf("%02d:%02d.%d %04x:%04x %-2x %-2x %-4x %-2x %-2d %d\n",
			dev->bus, dev->dev, dev->func, dev->vendor, dev->device,
			combined_class, dev->revision, dev->iobase,
			dev->header_type, dev->interrupt_line, dev->interrupt_pin);
	}

	return rsize;
}

void pci_init() {
	log(LOG_INFO, "PCI devices:\n");
	for(uint8_t bus = 0; bus < 255; bus++) {
		for(uint8_t dev = 0; dev < 32; dev++) {
			for(uint8_t func = 0; func < 8; func++) {
				try_load_device(bus, dev, func);
			}
		}
	}

	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("pci", &sfs_cb);
}
