/* rtl8139.c: Driver for the RTL8139 NIC
 * Copyright © 2011 Fritz Grimpen
 * Copyright © 2011-2018 Lukas Martini
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

#ifdef ENABLE_RTL8139

#include <hw/rtl8139.h>
#include <hw/pci.h>

#include <log.h>
#include <portio.h>
#include <string.h>
#include <print.h>
#include <hw/interrupts.h>
#include <memory/kmalloc.h>
#include <net/ether.h>
#include <net/net.h>
#include <fs/sysfs.h>

static uint32_t vendor_device_combos[][2] = {
	{0x10ec, 0x8139},
	{(uint32_t)NULL}
};

// Register port numbers
#define REG_ID0 0x00
#define REG_ID4 0x04

#define REG_TRANSMIT_STATUS0 0x10
#define REG_TRANSMIT_ADDR0 0x20
#define REG_RECEIVE_BUFFER 0x30
#define REG_COMMAND 0x37
#define REG_CUR_READ_ADDR 0x38
#define REG_INTERRUPT_MASK 0x3C
#define REG_INTERRUPT_STATUS 0x3E
#define REG_TRANSMIT_CONFIGURATION 0x40
#define REG_RECEIVE_CONFIGURATION 0x44
#define REG_CONFIG1 0x52

// Control register values
#define CR_RESET				(1 << 4)
#define CR_RECEIVER_ENABLE			(1 << 3)
#define CR_TRANSMITTER_ENABLE			(1 << 2)
#define CR_BUFFER_IS_EMPTY			(1 << 0)

// Transmitter configuration
#define TCR_IFG_STANDARD			(3 << 24)
#define TCR_MXDMA_512				(5 << 8)
#define TCR_MXDMA_1024				(6 << 8)
#define TCR_MXDMA_2048				(7 << 8)

// Receiver configuration
#define RCR_MXDMA_512				(5 << 8)
#define RCR_MXDMA_1024				(6 << 8)
#define RCR_MXDMA_UNLIMITED			(7 << 8)
#define RCR_ACCEPT_BROADCAST			(1 << 3)
#define RCR_ACCEPT_MULTICAST			(1 << 2)
#define RCR_ACCEPT_PHYS_MATCH			(1 << 1)

// Interrupt status register
#define ISR_RECEIVE_BUFFER_OVERFLOW		(1 << 4)
#define ISR_TRANSMIT_OK				(1 << 2)
#define ISR_RECEIVE_OK				(1 << 0)

#define RX_BUFFER_SIZE 8192

#define int_out8(card, port, value) portio_out8(card ->device->iobase + port, value)
#define int_out16(card, port, value) portio_out16(card ->device->iobase + port, value)
#define int_out32(card, port, value) portio_out32(card ->device->iobase + port, value)

#define int_in8(card, port) portio_in8(card ->device->iobase + port)
#define int_in16(card, port) portio_in16(card->device->iobase + port)

struct rtl8139_card {
	pci_device_t* device;
	char mac_addr[6];

	char* rx_buffer;
	int rx_buffer_offset;

	char* tx_buffer;
	bool tx_buffer_used:1;
	uint8_t cur_buffer;

	net_device_t* net_device;
};

static int cards = 0;
static struct rtl8139_card rtl8139_cards[RTL8139_MAX_CARDS];
/*
static void send_cb(net_device_t *dev, uint8_t *data, size_t len)
{
	struct rtl8139_card *card = dev->data;
	if (len > 1500)
	{
		++dev->stats.tx_errors;
		return;
	}

	while (card->tx_buffer_used) log(LOG_DEBUG, "rtl8139: Wait for buffer\n");

	memcpy(card->tx_buffer, data, len);
	if (len < 60)
	{
		memset(card->tx_buffer + len, 0, 60 - len);
		len = 60;
	}

	uint8_t cur_buffer = card->cur_buffer++;
	card->cur_buffer %= 4;

	int_out32(card, REG_TRANSMIT_ADDR0 + (4 * cur_buffer), (uint32_t)card->tx_buffer);
	int_out32(card, REG_TRANSMIT_STATUS0 + (4 * cur_buffer), len);

	++dev->stats.tx_packets;
	dev->stats.tx_bytes += len;

	log(LOG_DEBUG, "rtl8139: Queued data for sending\n");
}
*/
static void transmit(struct rtl8139_card *card)
{
	log(LOG_DEBUG, "rtl8139: Transmitted data successfully\n");
	card->tx_buffer_used = 0;
}

/* Copied from tyndur */
static void receive(struct rtl8139_card *card)
{
	while (1)
	{
		uint8_t command = int_in8(card, REG_COMMAND);

		if (command & CR_BUFFER_IS_EMPTY)
			break;

		char *rx_buffer = card->rx_buffer + card->rx_buffer_offset;

		uint16_t pHeader = *((uint16_t *)rx_buffer);
		rx_buffer += 2;

		if ((pHeader & 1) == 0)
				break;

		uint16_t length = *((uint16_t *)rx_buffer);
		rx_buffer += 2;

		card->rx_buffer_offset += 4;

		if (length >= sizeof(ether_frame_hdr_t))
		{
			uint8_t *data = kmalloc(length - 4);
			if ((card->rx_buffer_offset + length - 4) >= RX_BUFFER_SIZE)
			{
				memcpy(data, rx_buffer, RX_BUFFER_SIZE - card->rx_buffer_offset);
				memcpy(data + RX_BUFFER_SIZE - card->rx_buffer_offset,
						card->rx_buffer,
						(length - 4) - (RX_BUFFER_SIZE - card->rx_buffer_offset));
			}
			else
				memcpy(data, rx_buffer, length - 4);

			++card->net_device->stats.rx_packets;
			card->net_device->stats.rx_bytes += length - 4;
			net_receive(card->net_device, NET_PROTO_ETH, length - 4, data);
		}

		card->rx_buffer_offset += length;
		card->rx_buffer_offset = (card->rx_buffer_offset + 3) & ~0x3;
		card->rx_buffer_offset %= RX_BUFFER_SIZE;

		int_out16(card, REG_CUR_READ_ADDR, card->rx_buffer_offset - 0x10);
	}
}

static void int_handler(isf_t *state)
{
	log(LOG_DEBUG, "rtl8139: Got interrupt\n");

	// FIXME
	struct rtl8139_card* card = &rtl8139_cards[0];

	// Find the card this IRQ is coming from
	/*for(int i = 0; i < cards; i++) {
		if(likely(state->interrupt == rtl8139_cards[i].device->interrupt_line + IRQ0)) {
			card = &rtl8139_cards[i];
		}
	}*/

	if(unlikely(card == NULL)) {
		log(LOG_ERR, "rtl8139: Could not locate card for interrupt. This shouldn't happen.\n");
		return;
	}

	uint16_t isr = int_in16(card, REG_INTERRUPT_STATUS);
	uint16_t new_isr = 0;

	if (isr & ISR_TRANSMIT_OK)
	{
		new_isr |= ISR_TRANSMIT_OK;
		transmit(card);
	}

	if (isr & ISR_RECEIVE_OK)
	{
		receive(card);
		new_isr |= ISR_RECEIVE_OK;
	}

	int_out16(card, REG_INTERRUPT_STATUS, new_isr);
}
/*
static size_t sfs_write(void* data, size_t len, size_t offset, void* meta) {
	serial_printf("sfs_write 1\n");
	return len;

	struct rtl8139_card *card = &rtl8139_cards[0];
	if (len > 1500)
	{
//		++dev->stats.tx_errors;
		return 0;
	}
	serial_printf("sfs_write 2\n");

	while (card->tx_buffer_used) log(LOG_DEBUG, "rtl8139: Wait for buffer\n");
	serial_printf("sfs_write 3\n");

	memcpy(card->tx_buffer, data, len);
	if (len < 60)
	{
		memset(card->tx_buffer + len, 0, 60 - len);
		len = 60;
	}

	uint8_t cur_buffer = card->cur_buffer++;
	serial_printf("sfs_write 4, cur_buffer %d\n", cur_buffer);
	card->cur_buffer %= 4;
	serial_printf("sfs_write 5, tx_buffer at 0x%x\n", card->tx_buffer);

	int_out32(card, REG_TRANSMIT_ADDR0 + (4 * cur_buffer), (uint32_t)card->tx_buffer);
	int_out32(card, REG_TRANSMIT_STATUS0 + (4 * cur_buffer), len);
	serial_printf("sfs_write 6\n");

	printf("rtl8139 sent.\n");
	return len;
}
*/

static void enable(struct rtl8139_card *card)
{
	// Power on (LWAKE + LWPTN to active high), reset card
	int_out8(card, REG_CONFIG1, 0);
	int_out8(card, REG_COMMAND, CR_RESET);

	// Wait until reset is finished
	while((int_in8(card, REG_COMMAND) & REG_COMMAND) == CR_RESET);

	// Load MAC address
	bzero(&card->mac_addr, 6);
	for(int i = 0; i < 6; i++) {
		card->mac_addr[i] = int_in8(card, i);
	}

	if(card->device->interrupt_line == 0xff) {
		log(LOG_ERR, "rtl8139: Error: Card isn't connected to the PIC.");
		return;
	}
	interrupts_register(card->device->interrupt_line + IRQ0, int_handler, false);

	// Enable all interrupt events
	int_out16(card, REG_INTERRUPT_MASK, 0x0005);
	int_out16(card, REG_INTERRUPT_STATUS, 0);

	// Initialize RCR/TCR
	// RBLEN = 00, 8K + 16 bytes rx ring buffer
	int_out32(card, REG_RECEIVE_CONFIGURATION,
			RCR_MXDMA_UNLIMITED |
			RCR_ACCEPT_BROADCAST |
			RCR_ACCEPT_PHYS_MATCH);

	int_out32(card, REG_TRANSMIT_CONFIGURATION,
			TCR_IFG_STANDARD |
			TCR_MXDMA_2048);

	card->rx_buffer = (char *)kmalloc_a(8192 + 16);
	bzero(card->rx_buffer, 8192 + 16);
	card->rx_buffer_offset = 0;
	int_out32(card, REG_RECEIVE_BUFFER, (uint32_t)card->rx_buffer);
	serial_printf("receive buffer is at 0x%x\n", card->rx_buffer);

	card->tx_buffer = (char *)kmalloc_a(4096 + 16);
	bzero(card->tx_buffer, 4096 + 16);
	card->cur_buffer = 0;
	card->tx_buffer_used = false;

	// Enable receiver and transmitter
	int_out8(card, REG_COMMAND, CR_RECEIVER_ENABLE | CR_TRANSMITTER_ENABLE);
	//sysfs_add_dev("rtl1", NULL, sfs_write);

/*
	card->net_device = kmalloc(sizeof(net_device_t));
	snprintf(card->net_device->name, 15, "eth%d", ++net_ether_offset);
	memcpy(card->net_device->hwaddr, card->mac_addr, 6);
	card->net_device->mtu = 1500;
	card->net_device->proto = NET_PROTO_ETH;
	card->net_device->send = send_cb;
	card->net_device->data = card;

	net_register_device(card->net_device);
*/
}

void rtl8139_init()
{
	memset(rtl8139_cards, 0, RTL8139_MAX_CARDS * sizeof(struct rtl8139_card));

	pci_device_t** devices = (pci_device_t**)kmalloc(sizeof(void*) * RTL8139_MAX_CARDS);
	uint32_t ndevices = pci_search(devices, vendor_device_combos, RTL8139_MAX_CARDS);

	log(LOG_INFO, "rtl8139: Discovered %d devices.\n", ndevices);

	int i;
	for(i = 0; i < ndevices; ++i)
	{
		rtl8139_cards[i].device = devices[i];
		enable(&rtl8139_cards[i]);

		log(LOG_INFO, "rtl8139: %d:%d.%d, iobase 0x%x, irq %d, MAC Address %x:%x:%x:%x:%x:%x\n",
				devices[i]->bus,
				devices[i]->dev,
				devices[i]->func,
				devices[i]->iobase,
				devices[i]->interrupt_line,
				rtl8139_cards[i].mac_addr[0],
				rtl8139_cards[i].mac_addr[1],
				rtl8139_cards[i].mac_addr[2],
				rtl8139_cards[i].mac_addr[3],
				rtl8139_cards[i].mac_addr[4],
				rtl8139_cards[i].mac_addr[5]
			 );

		++cards;
	}
}

#endif /* ENABLE_RTL8139 */
