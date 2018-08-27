#pragma once

/* Copyright © 2011 Fritz Grimpen
 * Copyright © 2011, 2013 Lukas Martini
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


#define PCI_MAX_BUS  255
#define PCI_MAX_DEV  32
#define PCI_MAX_FUNC 8

enum pci_class {
	// Devices built before class codes (i.e. pre PCI 2.0)
	PCI_CLASS_OLD = 0x00,
	// Mass storage controller
	PCI_CLASS_STORAGE = 0x01,
	// Network controller
	PCI_CLASS_NETWORK = 0x02,
	// Display controller
	PCI_CLASS_DISPLAY = 0x03,
	// Multimedia device
	PCI_CLASS_MULTIMEDIA = 0x04,
	// Memory Controller
	PCI_CLASS_MEMORY = 0x05,
	// Bridge device
	PCI_CLASS_BRIDGE = 0x06,
	// Simple communications controllers
	PCI_CLASS_SIMCOM = 0x07,
	// Base system peripherals
	PCI_CLASS_SYSPERIPHERAL = 0x08,
	// Input devices
	PCI_CLASS_INPUT = 0x09,
	// Docking Stations
	PCI_CLASS_DOCK = 0x0A,
	// Processors
	PCI_CLASS_PROCESSOR = 0x0B,
	// Serial bus controllers
	PCI_CLASS_SERIAL = 0x0C,
	// Misc
	PCI_CLASS_MISC = 0xFF
};

typedef struct {
	uint16_t vendorID;
	uint16_t deviceID;

	uint8_t bus;
	uint8_t dev;
	uint8_t func;

	uint8_t revision;

	enum pci_class class;
	uint32_t iobase;
	uint32_t membase;
	uint8_t headerType;

	uint8_t interruptPin;
	uint8_t interruptLine;
} pci_device_t;

pci_device_t pci_devices[65536];

#define PCI_EXPAND(device) device.bus, device.dev, device.func
#define PCI_EXPAND_PTR(device) device->bus, device->dev, device->func

uint32_t _pci_config_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
uint16_t _pci_config_read16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
void _pci_config_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val);
void _pci_config_write16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t val);

#define _pci_get_vendor_id(args...) (uint16_t)_pci_config_read(args, 0)
#define _pci_get_header_type(args...) ((uint16_t)_pci_config_read(args, 0xe) & 127)

#define pci_config_read(device, offset) _pci_config_read(PCI_EXPAND_PTR(device), offset)
#define pci_config_read16(device, offset) _pci_config_read16(PCI_EXPAND_PTR(device), offset)
#define pci_config_write(device, offset, val) _pci_config_write(PCI_EXPAND_PTR(device), offset, val)
#define pci_config_write16(device, offset, val) _pci_config_write16(PCI_EXPAND_PTR(device), offset, val)
#define pci_get_device_id(device) (uint16_t)pci_config_read(device, 2)
#define pci_get_vendor_id(device) (uint16_t)pci_config_read(device, 0)
#define pci_get_interrupt_pin(device) (uint16_t)pci_config_read(device, 0x3d)
#define pci_get_interrupt_line(device) (uint16_t)pci_config_read(device, 0x3c)
#define pci_get_revision(device) (uint16_t)pci_config_read(device, 0x8)
#define pci_get_class(device) ((uint16_t)pci_config_read(device, 0x8) >> 8)
#define pci_get_header_type(device) ((uint16_t)pci_config_read(device, 0xe) & 127)

uint32_t pci_get_IO_base(pci_device_t* device);
uint32_t pci_get_BAR(pci_device_t* device, uint8_t bar);

void pci_loadDevice(pci_device_t *device, uint8_t bus, uint8_t dev, uint8_t func);
uint32_t pci_search_by_id(pci_device_t** returnDevices, uint32_t vendor_device_combos[][2], uint32_t maxNum);
uint32_t pci_search_by_class(pci_device_t** returnDevices, uint32_t class, uint32_t maxNum);

void pci_init();

