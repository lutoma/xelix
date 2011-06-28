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


struct rtl8139_card {
	pci_device_t *device;
	char mac_addr[6];
};

static struct rtl8139_card rtl8139_cards[1024];

static void rtl8139_enableCard(struct rtl8139_card *card)
{
	/* Load MAC address */
	memset(&card->mac_addr, 0, 6);
	card->mac_addr[0] = inb(card->device->iobase);
	card->mac_addr[1] = inb(card->device->iobase + 1);
	card->mac_addr[2] = inb(card->device->iobase + 2);
	card->mac_addr[3] = inb(card->device->iobase + 3);
	card->mac_addr[4] = inb(card->device->iobase + 4);
	card->mac_addr[5] = inb(card->device->iobase + 5);
}

void rtl8139_init()
{
	memset(rtl8139_cards, 0, 1024 * sizeof(struct rtl8139_card));
	int i = 0;
	int j = 0;
	while (i < 65536 && j < 1024)
	{
		if (pci_devices[i].vendor_id == 0x10ec && pci_devices[i].device_id == 0x8139)
		{
			rtl8139_cards[j].device = pci_devices + i;
			rtl8139_enableCard(rtl8139_cards + j);

			log("rtl8139: %d:%d.%d: MAC Address %x:%x:%x:%x:%x:%x\n",
					pci_devices[i].bus,
					pci_devices[i].dev,
					pci_devices[i].func,
					rtl8139_cards[j].mac_addr[0],
					rtl8139_cards[j].mac_addr[1],
					rtl8139_cards[j].mac_addr[2],
					rtl8139_cards[j].mac_addr[3],
					rtl8139_cards[j].mac_addr[4],
					rtl8139_cards[j].mac_addr[5]
				 );
			j++;
		}
		i++;
	}
}
