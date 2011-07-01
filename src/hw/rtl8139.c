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
#include <interrupts/interface.h>
#include <memory/kmalloc.h>

struct rtl8139_card {
	pci_device_t *device;
	char mac_addr[6];
	char *rx_buffer;
};

static struct rtl8139_card rtl8139_cards[1024];

static void rtl8139_intHandler(cpu_state_t *state)
{
}

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

	interrupts_registerHandler(card->device->interrupt_line, rtl8139_intHandler);

	outb(card->device->iobase + 0x37, 2 << 3);
	while ((inb(card->device->iobase + 0x37) & 16) != 0)
		printf("V");
	outb(card->device->iobase + 0x37, 2 << 2 | 2 << 1);

	outl(card->device->iobase + 0x40, 0x03000700);
	outl(card->device->iobase + 0x44, 0x0000070a);

	card->rx_buffer = (char *)kmalloc(8192 + 16);
	outl(card->device->iobase + 0x30, (uint32_t)card->rx_buffer);

	outw(card->device->iobase + 0x3e, 0);
	outw(card->device->iobase + 0x3c, 0xffff);
}

void rtl8139_init()
{
	memset(rtl8139_cards, 0, 1024 * sizeof(struct rtl8139_card));

	pci_device_t** devices = (pci_device_t**)kmalloc(sizeof(void*) * 1024);
	uint32_t numDevices = pci_searchDevice(devices, 0x10ec, 0x8139, 1024);
	
	log("rtl8139: Discovered %d device(s).\n", numDevices);
	
	int i;
	for(i = 0; i < numDevices; i++)
	{
		rtl8139_cards[i].device = devices[i];
		rtl8139_enableCard(&rtl8139_cards[i]);

		log("rtl8139: %d:%d.%d: MAC Address %x:%x:%x:%x:%x:%x\n",
				devices[i]->bus,
				devices[i]->dev,
				devices[i]->func,
				rtl8139_cards[i].mac_addr[0],
				rtl8139_cards[i].mac_addr[1],
				rtl8139_cards[i].mac_addr[2],
				rtl8139_cards[i].mac_addr[3],
				rtl8139_cards[i].mac_addr[4],
				rtl8139_cards[i].mac_addr[5]
			 );
	}
}
