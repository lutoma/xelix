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

// See src/drivers/bus/pci.c:[223:312] of
// de2c63d437317cd9d042e1a6e6a93c0cc78859d7 of
// git://git.etherboot.org/scm/gpxe.git

#define PCI_CONFIG_DATA    0x0CFC
#define PCI_CONFIG_ADDRESS 0x0CF8

static inline int pci_getAddress(int bus, int dev, int func, int offset)
{
	return 0x80000000|(bus<<16)|(dev<<11)|(func<<8)|(offset&0xFC);
}

int pci_configRead(int bus, int dev, int func, int offset)
{
  outl(PCI_CONFIG_ADDRESS, pci_getAddress(bus, dev, func, offset));
	return inl(PCI_CONFIG_DATA);
}

void pci_configWrite(int bus, int dev, int func, int offset, int val)
{
	outl(PCI_CONFIG_ADDRESS, pci_getAddress(bus, dev, func, offset));
	outl(PCI_CONFIG_DATA, val);
}

static inline int bitsum(int fullregister, int startbit, int stopbit)
{
	int summe;
	int stelle;
	int zielstelle;
	int tmp;
	fullregister = fullregister >> startbit;
	summe = 0;
	stelle = 1;
	zielstelle = POW2(stopbit-startbit) + 1;
	while (stelle < zielstelle)
	{
		tmp = fullregister & stelle;
		summe = summe + tmp;
		stelle = stelle*2;
	}
	return summe;
}

int pci_getVendorId(int bus,int dev,int func)
{
	int fullreg;
	int vendor_id;
	fullreg = pci_configRead(bus, dev, func, 0x000);
	vendor_id = bitsum(fullreg, 0, 15);
	return vendor_id;
}
