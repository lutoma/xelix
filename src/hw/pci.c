/* pci.c: PCI stack
 * Copyright © 2011 Barbers
 * Copyright © 2011 Fritz Grimpen
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

/* This file follows the following convention:
 *
 * All functions prefixed by an underscore expect a bus, device & function
 * number as argument and should generally only be used internally. All
 * other functions take a pci_device_t* as argument (from which they will
 * extract bus, device & function themselves).
 */

#include "pci.h"
#include <log.h>
#include <string.h>

// See src/drivers/bus/pci.c:[223:312] of
// de2c63d437317cd9d042e1a6e6a93c0cc78859d7 of
// git://git.etherboot.org/scm/gpxe.git

#define PCI_CONFIG_DATA    0x0CFC
#define PCI_CONFIG_ADDRESS 0x0CF8

static pci_device_t devices[65536];

static inline int get_address(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
	return 0x80000000|(bus<<16)|(dev<<11)|(func<<8)|(offset&0xFC);
}

uint32_t _pci_config_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
  outl(PCI_CONFIG_ADDRESS, get_address(bus, dev, func, offset));
	if (offset % 4 == 0)
		return inl(PCI_CONFIG_DATA);

	return (inl(PCI_CONFIG_DATA) >> ((offset % 4) * 8)) & 0xffff;
}

uint16_t _pci_config_read16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
	outw(PCI_CONFIG_ADDRESS, get_address(bus, dev, func, offset));
	return inw(PCI_CONFIG_DATA);
}

void _pci_config_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t val)
{
	outl(PCI_CONFIG_ADDRESS, get_address(bus, dev, func, offset));
	outl(PCI_CONFIG_DATA, val);
}

void _pci_config_write16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t val)
{
	outw(PCI_CONFIG_ADDRESS, get_address(bus, dev, func, offset));
	outw(PCI_CONFIG_DATA, val);
}

// *trollface*
// bar ∈ { x | x ∈ ℕ ∧ x ∈ [0, 5] }
uint32_t pci_get_BAR(pci_device_t* device, uint8_t bar)
{
	if (bar >= 6)
		return 0;

	uint8_t headerType = pci_get_header_type(device);
	if (headerType == 0x2 || (headerType == 0x1 && bar < 2))
		return 0;

	uint8_t _register = 0x10 + 0x4 * bar;
	return pci_config_read(device, _register);
}

uint32_t pci_get_IO_base(pci_device_t* device)
{
	uint8_t bars = 6 - pci_get_header_type(device) * 4;
	uint8_t i = 0;
	uint32_t bar;

	while (i < bars)
	{
		bar = pci_get_BAR(device, i++);
		if (bar & 0x1)
			return bar & 0xfffffffc;
	}

	return 0;
}

// TODO Write correct code (This is not correct ;))
uint32_t pci_get_mem_base(pci_device_t* device)
{
	uint8_t bars = 6 - pci_get_header_type(device) * 4;
	uint8_t i = 0;
	uint32_t bar;

	while ( bars > i)
	{
		bar = pci_get_BAR(device, i++);
		if ((bar & 0x1) == 0)
			return bar & 0xfffffff0;
	}

	return 0;
}

void pci_load_device(pci_device_t *device, uint8_t bus, uint8_t dev, uint8_t func)
{
	device->bus = bus;
	device->dev = dev;
	device->func = func;

	device->vendorID = pci_get_vendor_id(device);
	device->deviceID = pci_get_device_id(device);
	device->revision = pci_get_revision(device);
	device->class = pci_get_class(device);
	device->iobase = pci_get_IO_base(device);
	device->membase = pci_get_mem_base(device);
	device->headerType = pci_get_header_type(device);
	device->interruptPin = pci_get_interrupt_pin(device);
	device->interruptLine = pci_get_interrupt_line(device);
}

/* Searches a PCI device by vendor and device IDs.
 * returnDevices should be an allocated empty array, the size of which should be specified in maxNum.
 * vendor_device_combos should be an array of the format
 *
 * static uint32_t vendor_device_combos[][2] = {
 * 	{vendor_id, device_id},
 * 	{vendor_id, device_id},
 * 	{NULL}
 * };
 *
 * Please don't forget the NULL in the end. Pretty please.
 */
uint32_t pci_search_by_id(pci_device_t** returnDevices, uint32_t vendor_device_combos[][2], uint32_t maxNum)
{

	if(!vendor_device_combos[0] || !vendor_device_combos[0][0])	{
		return 0;
	}

	/* FIXME Should be a binary search or something else more reasonable
	 * We could probably also sort the devices by ID in the first place
	 */

	int devices_found = 0;

	for(int i = 0; vendor_device_combos[i][0]; i++)
	{
		for(int device = 0; device < 65536 && devices_found < maxNum; device++)
		{
			if (devices[device].vendorID != vendor_device_combos[i][0] ||
				devices[device].deviceID != vendor_device_combos[i][1])
				continue;

			returnDevices[devices_found] = &devices[device];
			devices_found++;
		}
	}

	return devices_found;
}

uint32_t pci_search_by_class(pci_device_t** returnDevices, uint32_t class, uint32_t maxNum)
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
				vendor = _pci_get_vendor_id(bus, dev, func);

				/* Devices which don't exist should have a vendor of 0xffff,
				 * however, some weird chipsets also wrongly set it to zero.
				 */
				if (vendor == 0xffff || vendor == 0)
					continue;

				pci_load_device(devices + i, bus, dev, func);
				printf("    %d:%d.%d: [%x:%x] %-2x rev %-2x class %-2x iobase %-4x type %-2x int %-2d pin %-2d\n",
						devices[i].bus,
						devices[i].dev,
						devices[i].func,
						devices[i].vendorID,
						devices[i].deviceID,
						devices[i].class,
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
