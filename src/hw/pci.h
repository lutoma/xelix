#pragma once

/* Copyright © 2011 Fritz Grimpen
 * Copyright © 2011 Lukas Martini
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

#include <lib/generic.h>

#define PCI_MAX_BUS  255
#define PCI_MAX_DEV  32
#define PCI_MAX_FUNC 8

typedef struct {
	uint16_t vendorID;
	uint16_t deviceID;

	uint8_t bus;
	uint8_t dev;
	uint8_t func;

	uint8_t revision;

	uint32_t class;
	uint32_t iobase;
	uint32_t membase;
	uint8_t headerType;

	uint8_t interruptPin;
	uint8_t interruptLine;
} pci_device_t;

pci_device_t pci_devices[65536];

#define PCI_EXPAND(device) device.bus, device.dev, device.func

uint32_t pci_configRead(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
void pci_configWrite(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val);

uint16_t pci_getVendorId(uint8_t bus, uint8_t dev, uint8_t func);
uint16_t pci_getDeviceId(uint8_t bus, uint8_t dev, uint8_t func);
uint8_t pci_getRevision(uint8_t bus, uint8_t dev, uint8_t func);
uint32_t pci_getClass(uint8_t bus, uint8_t dev, uint8_t func);
uint8_t pci_getHeaderType(uint8_t bus, uint8_t dev, uint8_t func);
uint32_t pci_getBAR(uint8_t bus, uint8_t dev, uint8_t func, uint8_t bar);
uint32_t pci_getIOBase(uint8_t bus, uint8_t dev, uint8_t func);
uint32_t pci_getMemBase(uint8_t bus, uint8_t dev, uint8_t func);
uint8_t pci_getInterruptPin(uint8_t bus, uint8_t dev, uint8_t func);
uint8_t pci_getInterruptLine(uint8_t bus, uint8_t dev, uint8_t func);
void pci_loadDevice(pci_device_t *device, uint8_t bus, uint8_t dev, uint8_t func);
uint32_t pci_searchDevice(pci_device_t** devices, uint16_t vendorId, uint16_t deviceId, uint32_t maxNum);
uint32_t pci_searchByClass(pci_device_t** returnDevices, uint32_t class, uint32_t maxNum);

void pci_init();

