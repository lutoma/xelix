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

static inline int pci_getAddress(int bus, int dev, int func, int offset)
{
	return 0x80000000|(bus<<16)|(dev<<11)|(func<<8)|(offset&0xFC);
}

uint32_t pci_configRead(uint16_t bus, uint16_t dev, uint16_t func, uint16_t offset)
{
  outl(PCI_CONFIG_ADDRESS, pci_getAddress(bus, dev, func, offset));
	return (inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xffff;
}

void pci_configWrite(uint16_t bus, uint16_t dev, uint16_t func, uint16_t offset, uint32_t val)
{
	outl(PCI_CONFIG_ADDRESS, pci_getAddress(bus, dev, func, offset));
	outl(PCI_CONFIG_DATA, val);
}

void pci_init()
{
	pci_scan();
}

void pci_scan()
{
	uint16_t bus = 0;
	uint16_t device = 0;
	uint16_t vendor_id;
	uint16_t device_id;

	while (bus < 1)
	{
		device = 0;
		while (device < 65535)
		{
			vendor_id = pci_getVendorId(bus, device, 0);
			device_id = pci_getDeviceId(bus, device, 0);

			if (vendor_id != 0xffff)
				log("Detected PCI device %d [%x:%x] at bus %d\n", device, vendor_id, device_id, bus);

			device++;
		}
		bus++;
	}
}

uint16_t pci_getDeviceId(uint16_t bus, uint16_t dev, uint16_t func)
{
	return (uint16_t)pci_configRead(bus, dev, func, 2);
}

uint16_t pci_getVendorId(uint16_t bus, uint16_t dev, uint16_t func)
{
	return (uint16_t)pci_configRead(bus, dev, func, 0x000);
}
