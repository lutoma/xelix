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

void pci_init()
{
	memset(pci_devices, 0xff, 65536 * sizeof(pci_device_t));
	int i = 0;
	uint8_t bus, dev, func;
	uint16_t vendor, device;

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
					device = pci_getDeviceId(bus, dev, func);

					pci_devices[i].bus = bus;
					pci_devices[i].dev = dev;
					pci_devices[i].func = func;
					pci_devices[i].vendor_id = vendor;
					pci_devices[i].device_id = device;
					pci_devices[i].revision = pci_getRevision(bus, dev, func);
					pci_devices[i].class = pci_getClass(bus, dev, func);

					log("pci: %d:%d.%d: Unknown Device [%x:%x] Revision %x Class %x\n", bus, dev, func, vendor, device, pci_devices[i].revision, pci_devices[i].class);
					i++;
				}

				func++;
			}
			dev++;
		}
		bus++;
	}
}
