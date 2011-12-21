/* rtl8139.c: Driver for the RTL8139 NIC
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

#include <hw/rtl8139.h>
#include <hw/pci.h>

#include <lib/log.h>
#include <lib/portio.h>
#include <interrupts/interface.h>
#include <memory/kmalloc.h>
#include <net/ether.h>
#include <net/net.h>
#include <lib/string.h>

#define VENDOR_ID 0x10ec
#define DEVICE_ID 0x8139

// This driver will only support that many cards
#define MAX_CARDS 50 

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
	pci_device_t *device;
	char macAddr[6];

	char *rxBuffer;
	int rxBufferOffset;

	char *txBuffer;
	bool txBufferUsed:1;
	uint8_t curBuffer;

	net_device_t *netDevice;
};

static int cards = 0;
static struct rtl8139_card rtl8139_cards[MAX_CARDS];

static void sendCallback(net_device_t *dev, uint8_t *data, size_t len)
{
	struct rtl8139_card *card = dev->data;

	while (card->txBufferUsed) log(LOG_DEBUG, "rtl8139: Wait for buffer\n");

	memcpy(card->txBuffer, data, len);
	if (len < 60)
	{
		memset(card->txBuffer + len, 0, 60 - len);
		len = 60;
	}

	uint8_t curBuffer = card->curBuffer++;
	card->curBuffer %= 4;

	int_out32(card, REG_TRANSMIT_ADDR0 + (4 * curBuffer), (uint32_t)card->txBuffer);
	int_out32(card, REG_TRANSMIT_STATUS0 + (4 * curBuffer), len);

	log(LOG_DEBUG, "rtl8139: Queued data for sending\n");
}

static void transmitData(struct rtl8139_card *card)
{
	log(LOG_DEBUG, "rtl8139: Transmitted data successfully\n");
	card->txBufferUsed = 0;
}

/* Copied from tyndur */
static void receiveData(struct rtl8139_card *card)
{
	while (1)
	{
		uint8_t command = int_in8(card, REG_COMMAND);

		if (command & CR_BUFFER_IS_EMPTY)
			break;

		char *rxBuffer = card->rxBuffer + card->rxBufferOffset;

		uint16_t pHeader = *((uint16_t *)rxBuffer);
		rxBuffer += 2;

		if ((pHeader & 1) == 0)
				break;
		
		uint16_t length = *((uint16_t *)rxBuffer);
		rxBuffer += 2;

		card->rxBufferOffset += 4;

		if (length >= sizeof(ether_frame_hdr_t))
		{
			uint8_t *data = kmalloc(length - 4);
			if ((card->rxBufferOffset + length - 4) >= RX_BUFFER_SIZE)
			{
				memcpy(data, rxBuffer, RX_BUFFER_SIZE - card->rxBufferOffset);
				memcpy(data + RX_BUFFER_SIZE - card->rxBufferOffset,
						card->rxBuffer,
						(length - 4) - (RX_BUFFER_SIZE - card->rxBufferOffset));
			}
			else
				memcpy(data, rxBuffer, length - 4);

			net_receive(card->netDevice, NET_PROTO_ETH, length - 4, data);
		}
		
		card->rxBufferOffset += length;
		card->rxBufferOffset = (card->rxBufferOffset + 3) & ~0x3;
		card->rxBufferOffset %= RX_BUFFER_SIZE;

		int_out16(card, REG_CUR_READ_ADDR, card->rxBufferOffset - 0x10);
	}
}

static void rtl8139_intHandler(cpu_state_t *state)
{
	for (int i = 0; i < cards; ++i)
	{
		struct rtl8139_card *card = rtl8139_cards + i;
		uint16_t isr = int_in16(card, REG_INTERRUPT_STATUS);
		uint16_t new_isr = 0;

		if (isr & ISR_TRANSMIT_OK)
		{
			new_isr |= ISR_TRANSMIT_OK;
			transmitData(card);
		}

		if (isr & ISR_RECEIVE_OK)
		{
			receiveData(card);
			new_isr |= ISR_RECEIVE_OK;
		}

		int_out16(card, REG_INTERRUPT_STATUS, new_isr);
	}
}

static void enableCard(struct rtl8139_card *card)
{
	// Power on! (LWAKE + LWPTN to active high)
	int_out8(card, REG_CONFIG1, 0);

	// Reset card
	int_out8(card, REG_COMMAND, CR_RESET);

	// Wait until reset is finished
	while((int_in8(card, REG_COMMAND) & REG_COMMAND) == CR_RESET);
	log(LOG_DEBUG, "rtl8139: Reset finished.\n");

	// Load MAC address
	memset(&card->macAddr, 0, 6);
	card->macAddr[0] = int_in8(card, 0);
	card->macAddr[1] = int_in8(card, 1);
	card->macAddr[2] = int_in8(card, 2);
	card->macAddr[3] = int_in8(card, 3);
	card->macAddr[4] = int_in8(card, 4);
	card->macAddr[5] = int_in8(card, 5);
	log(LOG_DEBUG, "rtl8139: Got MAC address.\n");

	// Enable all interrupt events
	if(card->device->interruptLine == 0xFF)
	{
		log(LOG_DEBUG, "rtl8139: Error: Card isn't connected to the PIC.");
		return;
	}
	
	interrupts_registerHandler(card->device->interruptLine + IRQ0, rtl8139_intHandler);

	int_out16(card, REG_INTERRUPT_MASK,  0x0005);
	int_out16(card, REG_INTERRUPT_STATUS, 0);

	// Initialize RCR/TCR
	// RBLEN = 00, 8K + 16 bytes rx ring buffer
	int_out32(card, REG_RECEIVE_CONFIGURATION,
			RCR_MXDMA_UNLIMITED |
			RCR_ACCEPT_BROADCAST |
			RCR_ACCEPT_PHYS_MATCH
		);
	
	int_out32(card, REG_TRANSMIT_CONFIGURATION,
			TCR_IFG_STANDARD |
			TCR_MXDMA_2048
		);

	card->rxBuffer = (char *)kmalloc(8192 + 16 );
	memset(card->rxBuffer, 0, 8192 + 16);
	card->rxBufferOffset = 0;
	int_out32(card, REG_RECEIVE_BUFFER, (uint32_t)card->rxBuffer);

	card->txBuffer = (char *)kmalloc(4096 + 16);
	memset(card->txBuffer, 0, 4096 + 16);
	card->curBuffer = 0;
	card->txBufferUsed = false;

	// Enable receiver and transmitter
	int_out8(card, REG_COMMAND, CR_RECEIVER_ENABLE | CR_TRANSMITTER_ENABLE);
	log(LOG_DEBUG, "rtl8139: Enabled receiver / transmitter.\n");

	card->netDevice = kmalloc(sizeof(net_device_t));
	strcpy(card->netDevice->name, "eth");
	strcpy(card->netDevice->name + 3, itoa(net_ether_offset++, 10));
	memcpy(card->netDevice->hwaddr, card->macAddr, 6);
	card->netDevice->mtu = 1500;
	card->netDevice->proto = NET_PROTO_ETH;
	card->netDevice->send = sendCallback;
	card->netDevice->data = card;

	net_register_device(card->netDevice);
}

void rtl8139_init()
{
	memset(rtl8139_cards, 0, MAX_CARDS * sizeof(struct rtl8139_card));

	pci_device_t** devices = (pci_device_t**)kmalloc(sizeof(void*) * MAX_CARDS);
	uint32_t numDevices = pci_searchDevice(devices, VENDOR_ID, DEVICE_ID, MAX_CARDS);
	
	log(LOG_INFO, "rtl8139: Discovered %d device%p.\n", numDevices);
	
	int i;
	for(i = 0; i < numDevices; ++i)
	{
		rtl8139_cards[i].device = devices[i];
		enableCard(&rtl8139_cards[i]);

		log(LOG_INFO, "rtl8139: %d:%d.%d, iobase 0x%x, irq %d, MAC Address %x:%x:%x:%x:%x:%x\n",
				devices[i]->bus,
				devices[i]->dev,
				devices[i]->func,
				devices[i]->iobase,
				devices[i]->interruptLine,
				rtl8139_cards[i].macAddr[0],
				rtl8139_cards[i].macAddr[1],
				rtl8139_cards[i].macAddr[2],
				rtl8139_cards[i].macAddr[3],
				rtl8139_cards[i].macAddr[4],
				rtl8139_cards[i].macAddr[5]
			 );

		++cards;
	}
}
