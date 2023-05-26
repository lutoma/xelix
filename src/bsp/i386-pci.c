/* pci.c: PCI stack
 * Copyright © 2011 Barbers
 * Copyright © 2011 Fritz Grimpen
 * Copyright © 2011-2018 Lukas Martini
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
#include <log.h>
#include <string.h>
#include <portio.h>
#include <fs/sysfs.h>
#include <mem/kmalloc.h>

#define PCI_CONFIG_DATA    0x0CFC
#define PCI_CONFIG_ADDRESS 0x0CF8

#define get_header_type(device) ((uint16_t)pci_config_read(device, 0xe) & 127)

#define get_address(bus, dev, func, offset) (0x80000000 | (bus << 16) | \
	(dev << 11) | (func << 8) | (offset & 0xFC))

static pci_device_t* pci_devices = NULL;


uint32_t _pci_config_read(uint8_t bus, uint8_t dev, uint8_t func,
	uint8_t offset) {

	outl(PCI_CONFIG_ADDRESS, get_address(bus, dev, func, offset));
	if(!(offset % 4)) {
		return inl(PCI_CONFIG_DATA);
	}

	return (inl(PCI_CONFIG_DATA) >> ((offset % 4) * 8)) & 0xffff;
}

void _pci_config_write(uint8_t bus, uint8_t dev, uint8_t func,
	uint8_t offset, uint32_t val) {

	outl(PCI_CONFIG_ADDRESS, get_address(bus, dev, func, offset));
	outl(PCI_CONFIG_DATA, val);
}

uint32_t pci_get_BAR(pci_device_t* device, uint8_t bar) {
	if(bar > 5) {
		return 0;
	}

	uint8_t header_type = get_header_type(device);
	if(header_type == 0x2 || (header_type == 0x1 && bar < 2)) {
		return 0;
	}

	uint8_t _register = 0x10 + 0x4 * bar;
	return pci_config_read(device, _register);
}

static uint32_t get_IO_base(pci_device_t* device) {
	uint8_t bars = 6 - get_header_type(device) * 4;

	for(int i = 0; i < bars; i++) {
		uint32_t bar = pci_get_BAR(device, i);
		if(bar & 0x1) {
			return bar & 0xfffffffc;
		}
	}

	return 0;
}

static uint32_t get_mem_base(pci_device_t* device) {
	uint8_t bars = 6 - get_header_type(device) * 4;

	for(int i = 0; i < bars; i++) {
		uint32_t bar = pci_get_BAR(device, i++);
		if(!(bar & 0x1)) {
			return bar & 0xfffffff0;
		}
	}

	return 0;
}

static void load_device(pci_device_t *device, uint8_t bus, uint8_t dev, uint8_t func) {
	device->bus = bus;
	device->dev = dev;
	device->func = func;

	device->vendor = (uint16_t)pci_config_read(device, 0);
	device->device = (uint16_t)pci_config_read(device, 2);
	device->revision = (uint16_t)pci_config_read(device, 0x8);
	device->class = (uint16_t)(pci_config_read(device, 0x8) >> 16);
	device->iobase = get_IO_base(device);
	device->membase = get_mem_base(device);
	device->header_type = get_header_type(device);
	device->interrupt_pin = (uint16_t)pci_config_read(device, 0x3d);
	device->interrupt_line = (uint16_t)pci_config_read(device, 0x3c);
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
		sysfs_printf("%02d:%02d.%d %04x:%04x %-2x %-2x %-4x %-2x %-2d %d\n",
			dev->bus, dev->dev, dev->func, dev->vendor, dev->device,
			dev->class, dev->revision, dev->iobase,
			dev->header_type, dev->interrupt_line, dev->interrupt_pin);
	}

	return rsize;
}

void pci_init() {
	log(LOG_INFO, "PCI devices:\n");
	for(uint8_t bus = 0; bus < PCI_MAX_BUS; bus++) {
		for(uint8_t dev = 0; dev < PCI_MAX_DEV; dev++) {
			for(uint8_t func = 0; func < PCI_MAX_FUNC; func++) {
				uint16_t vendor = (uint16_t)_pci_config_read(bus, dev, func, 0);

				/* Devices which don't exist should have a vendor of 0xffff,
				 * however, some weird chipsets also wrongly set it to zero.
				 * XXX: ????
				 */
				if (vendor == 0xffff || vendor == 0)
					continue;

				pci_device_t* pdev = kmalloc(sizeof(pci_device_t));
				load_device(pdev, bus, dev, func);

				pdev->next = pci_devices;
				pci_devices = pdev;

				log(LOG_INFO, "  %02d:%02d.%d: %04x:%04x rev %-2x class %04x iobase %-4x type %-2x int %-2d pin %d\n",
						pdev->bus, pdev->dev, pdev->func, pdev->vendor,
						pdev->device, pdev->revision, pdev->class,
						pdev->iobase, pdev->header_type, pdev->interrupt_line,
						pdev->interrupt_pin);
			}
		}
	}

	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("pci", &sfs_cb);
}
