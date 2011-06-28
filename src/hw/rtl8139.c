/* rtl8139.c: Driver for the RTL8139 NIC
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

#include <hw/rtl8139.h>
#include <lib/log.h>

void rtl8139_init()
{
	int i = 0;
	while (i < 65536)
	{
		if (pci_devices[i].vendor_id == 0x10ec && pci_devices[i].device_id == 0x8139)
		{
			log("rtl8139: Detected RTL8139 at %d:%d.%d\n", pci_devices[i].bus, pci_devices[i].dev, pci_devices[i].func);
		}
		i++;
	}
}
