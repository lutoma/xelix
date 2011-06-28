/* pci.c: Simple PCI functions
 * Copyright © 2011 Barbers
 * Copyright © 2011 Fritz Grimpen
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

pci_device_t pci_devices[65536];

static inline int pci_getAddress(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
	return 0x80000000|(bus<<16)|(dev<<11)|(func<<8)|(offset&0xFC);
}

uint32_t pci_configRead(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
  outl(PCI_CONFIG_ADDRESS, pci_getAddress(bus, dev, func, offset));
	if ((offset & 2) == 0)
		return inl(PCI_CONFIG_DATA);

	return (inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xffff;
}

void pci_configWrite(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val)
{
	outl(PCI_CONFIG_ADDRESS, pci_getAddress(bus, dev, func, offset));
	outl(PCI_CONFIG_DATA, val);
}

uint16_t pci_getDeviceId(uint8_t bus, uint8_t dev, uint8_t func)
{
	return (uint16_t)pci_configRead(bus, dev, func, 2);
}

uint16_t pci_getVendorId(uint8_t bus, uint8_t dev, uint8_t func)
{
	return (uint16_t)pci_configRead(bus, dev, func, 0);
}

uint8_t pci_getRevision(uint8_t bus, uint8_t dev, uint8_t func)
{
	return pci_configRead(bus, dev, func, 0x8);
}

uint32_t pci_getClass(uint8_t bus, uint8_t dev, uint8_t func)
{
	return pci_configRead(bus, dev, func, 0x8) >> 8;
}

uint8_t pci_getHeaderType(uint8_t bus, uint8_t dev, uint8_t func)
{
	return pci_configRead(bus, dev, func, 0xe) & 127;	
}

// *trollface*
// bar ∈ { x | x ∈ ℕ ∧ x ∈ [0, 5] }
uint32_t pci_getBAR(uint8_t bus, uint8_t dev, uint8_t func, uint8_t bar)
{
	if (bar >= 6)
		return 0;

	uint8_t header_type = pci_getHeaderType(bus, dev, func);
	if (header_type == 0x2 || (header_type == 0x1 && bar < 2))
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

	while ( bars < i)
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
	device->vendor_id = pci_getVendorId(bus, dev, func);
	device->device_id = pci_getDeviceId(bus, dev, func);
	device->bus = bus;
	device->dev = dev;
	device->func = func;
	device->revision = pci_getRevision(bus, dev, func);
	device->class = pci_getClass(bus, dev, func);
	device->iobase = pci_getIOBase(bus, dev, func);
	device->membase = pci_getMemBase(bus, dev, func);
	device->header_type = pci_getHeaderType(bus, dev, func);
	device->interrupt_pin = pci_getInterruptPin(bus, dev, func);
	device->interrupt_line = pci_getInterruptLine(bus, dev, func);
}

void pci_init()
{
	memset(pci_devices, 0xff, 65536 * sizeof(pci_device_t));
	int i = 0;
	uint8_t bus, dev, func;
	uint16_t vendor;

	bus = 0;
	while (bus < PCI_MAX_BUS)
	{
		dev = 0;
		while (dev < PCI_MAX_DEV)
		{
			func = 0;
			while (func < PCI_MAX_FUNC)
			{
				vendor = pci_getVendorId(bus, dev, func);
				if (vendor != 0xffff)
				{
					pci_loadDevice(pci_devices + i, bus, dev, func);
					log("pci: %d:%d.%d: Unknown Device [%x:%x] (rev %x class %x iobase %x type %x int %d)\n",
							pci_devices[i].bus,
							pci_devices[i].dev,
							pci_devices[i].func,
							pci_devices[i].vendor_id,
							pci_devices[i].device_id,
							pci_devices[i].revision,
							pci_devices[i].class,
							pci_devices[i].iobase,
							pci_devices[i].header_type,
							pci_devices[i].interrupt_line);
					i++;
				}

				func++;
			}
			dev++;
		}
		bus++;
	}
}
