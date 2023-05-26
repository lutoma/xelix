#pragma once

/* Copyright © 2011 Fritz Grimpen
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

typedef struct pci_device {
	uint16_t vendor;
	uint16_t device;

	uint8_t bus;
	uint8_t dev;
	uint8_t func;

	uint8_t revision;

	enum pci_class class;
	uint32_t iobase;
	uint32_t membase;
	uint8_t header_type;

	uint8_t interrupt_pin;
	uint8_t interrupt_line;

	struct pci_device* next;
} pci_device_t;

#define pci_expand_dev(device) device->bus, device->dev, device->func
#define pci_config_read(device, offset) _pci_config_read(pci_expand_dev(device), offset)
#define pci_config_write(device, offset, val) \
	_pci_config_write(pci_expand_dev(device), offset, val)

uint32_t _pci_config_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
void _pci_config_write(uint8_t bus, uint8_t dev, uint8_t func,
	uint8_t offset, uint32_t val);


int pci_walk(int (*callback)(pci_device_t* dev));
int pci_check_vendor(pci_device_t* dev, const uint32_t combos[][2]);
uint32_t pci_get_BAR(pci_device_t* device, uint8_t bar);
void pci_init();

