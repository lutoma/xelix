/* pci.c: Simple PCI functions
 * Copyright © 2011 Barbers
 * Copyright © 2011 Fritz Grimpen
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

#include "pci.h"
#include <lib/log.h>

// See src/drivers/bus/pci.c:[223:312] of
// de2c63d437317cd9d042e1a6e6a93c0cc78859d7 of
// git://git.etherboot.org/scm/gpxe.git

#define PCI_CONFIG_DATA    0x0CFC
#define PCI_CONFIG_ADDRESS 0x0CF8

pci_device_t devices[65536];

static inline int getAddress(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
	return 0x80000000|(bus<<16)|(dev<<11)|(func<<8)|(offset&0xFC);
}

uint32_t pci_configRead(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
  outl(PCI_CONFIG_ADDRESS, getAddress(bus, dev, func, offset));
	if (offset % 4 == 0)
		return inl(PCI_CONFIG_DATA);

	return (inl(PCI_CONFIG_DATA) >> ((offset % 4) * 8)) & 0xffff;
}

void pci_configWrite(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val)
{
	outl(PCI_CONFIG_ADDRESS, getAddress(bus, dev, func, offset));
	outl(PCI_CONFIG_DATA, val);
}

// *trollface*
// bar ∈ { x | x ∈ ℕ ∧ x ∈ [0, 5] }
uint32_t pci_getBAR(uint8_t bus, uint8_t dev, uint8_t func, uint8_t bar)
{
	if (bar >= 6)
		return 0;

	uint8_t headerType = pci_getHeaderType(bus, dev, func);
	if (headerType == 0x2 || (headerType == 0x1 && bar < 2))
		return 0;

	uint8_t _register = 0x10 + 0x4 * bar;
	return pci_configRead(bus, dev, func, _register);
}

uint32_t pci_getIOBase(uint8_t bus, uint8_t dev, uint8_t func)
{
	uint8_t bars = 6 - pci_getHeaderType(bus, dev, func) * 4;
	uint8_t i = 0;
	uint32_t bar;

	while (i < bars)
	{
		bar = pci_getBAR(bus, dev, func, i++);
		if (bar & 0x1)
			return bar & 0xfffffffc;
	}

	return 0;
}

// TODO Write correct code (This is not correct ;))
uint32_t pci_getMemBase(uint8_t bus, uint8_t dev, uint8_t func)
{
	uint8_t bars = 6 - pci_getHeaderType(bus, dev, func) * 4;
	uint8_t i = 0;
	uint32_t bar;

	while ( bars > i)
	{
		bar = pci_getBAR(bus, dev, func, i++);
		if ((bar & 0x1) == 0)
			return bar & 0xfffffff0;
	}

	return 0;
}

uint8_t pci_getInterruptPin(uint8_t bus, uint8_t dev, uint8_t func)
{
	return pci_configRead(bus, dev, func, 0x3d);
}

uint8_t pci_getInterruptLine(uint8_t bus, uint8_t dev, uint8_t func)
{
	return pci_configRead(bus, dev, func, 0x3c);
}

void pci_loadDevice(pci_device_t *device, uint8_t bus, uint8_t dev, uint8_t func)
{
	device->vendorID = pci_getVendorId(bus, dev, func);
	device->deviceID = pci_getDeviceId(bus, dev, func);
	device->bus = bus;
	device->dev = dev;
	device->func = func;
	device->revision = pci_getRevision(bus, dev, func);
	device->class = pci_getClass(bus, dev, func);
	device->iobase = pci_getIOBase(bus, dev, func);
	device->membase = pci_getMemBase(bus, dev, func);
	device->headerType = pci_getHeaderType(bus, dev, func);
	device->interruptPin = pci_getInterruptPin(bus, dev, func);
	device->interruptLine = pci_getInterruptLine(bus, dev, func);
}

uint32_t pci_searchDevice(pci_device_t** returnDevices, uint16_t vendorId, uint16_t deviceId, uint32_t maxNum)
{
	int i = 0;
	int j = 0;

	for(; i < 65536 && j < maxNum; i++)
	{
		if (devices[i].vendorID != vendorId || devices[i].deviceID != deviceId)
			continue;

		returnDevices[j] = &devices[i];
		j++;
	}

	return j;
}

uint32_t pci_searchByClass(pci_device_t** returnDevices, uint32_t class, uint32_t maxNum)
{
	int i = 0;
	int j = 0;

	for(; i < 65536 && j < maxNum; i++)
	{
		if (devices[i].class != class)
			continue;

		returnDevices[j] = &devices[i];
		j++;
	}

	return j;
}


void pci_init()
{
	memset(devices, 0xff, 65536 * sizeof(pci_device_t));
	int i = 0;
	uint8_t bus, dev, func;
	uint16_t vendor;

	for(bus = 0; bus < PCI_MAX_BUS; bus++)
	{
		for(dev = 0; dev < PCI_MAX_DEV; dev++)
		{
			for(func = 0; func < PCI_MAX_FUNC; func++)
			{
				vendor = pci_getVendorId(bus, dev, func);
				if (vendor == 0xffff)
					continue;
					
				pci_loadDevice(devices + i, bus, dev, func);
				log(LOG_INFO, "pci: %d:%d.%d: Unknown Device [%x:%x] (rev %x class %x iobase %x type %x int %d pin %d)\n",
						devices[i].bus,
						devices[i].dev,
						devices[i].func,
						devices[i].vendorID,
						devices[i].deviceID,
						devices[i].revision,
						devices[i].class,
						devices[i].iobase,
						devices[i].headerType,
						devices[i].interruptLine,
						devices[i].interruptPin);
				i++;
			}
		}
	}
}
