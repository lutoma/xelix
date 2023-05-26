/* ne2k.c: Driver for Ne2000 class NICs
 * Copyright © 2018-2019 Lukas Martini
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

#ifdef CONFIG_ENABLE_NE2K

#include <net/i386-ne2k.h>
#include <net/net.h>
#include <bsp/i386-pci.h>
#include <log.h>
#include <portio.h>
#include <bitmap.h>
#include <string.h>
#include <printf.h>
#include <fs/sysfs.h>
#include <int/int.h>
#include <mem/kmalloc.h>
#include <tasks/task.h>
#include <pico_device.h>

#define RECV_BUFFER_SIZE 2048

// Page 0 registers
#define R_CR		0x00
#define R_PSTART	0x01
#define R_PSTOP		0x02
#define R_BNRY		0x03
#define R_TPSR		0x04
#define R_TBCR0		0x05
#define R_TBCR1		0x06
#define R_ISR		0x07
#define R_RSAR0		0x08
#define R_RSAR1		0x09
#define R_RBCR0		0x0a
#define R_RBCR1		0x0b
#define R_RCR		0x0c
#define R_TCR		0x0d
#define R_DCR		0x0e
#define R_IMR		0x0f

// Page 1
#define R_CURR		0x07

// CR register flags
#define CR_STOP		(1 << 0)
#define CR_START	(1 << 1)
#define CR_TRANSMIT	(1 << 2)
#define CR_RD0		(1 << 3)
#define CR_RD1		(1 << 4)
#define CR_RD2		(1 << 5)
#define CR_PAGE1	(1 << 6)
#define CR_PAGE2	(1 << 7)

#define CR_DMA_NO		0
#define CR_DMA_READ		CR_RD0
#define CR_DMA_WRITE	CR_RD1
#define CR_DMA_SEND		(CR_RD0 | CR_RD1)
#define CR_DMA_ABORT	CR_RD2

#define MEM_TRANSMIT	0x4000
#define MEM_RECEIVE		0x5000
#define MEM_END			0x8000

#define PAGE_TRANSMIT	(MEM_TRANSMIT >> 8)
#define PAGE_RECEIVE	(MEM_RECEIVE >> 8)
#define PAGE_END		(MEM_END >> 8)

#define ioutb(pin, val) outb(dev->iobase + (pin), val)
#define ioutw(pin, val) outw(dev->iobase + (pin), val)
#define iinb(pin) inb(dev->iobase + (pin))
#define iinw(pin) inw(dev->iobase + (pin))

struct recv_frame_header {
	uint8_t recv_status;
	uint8_t next;
	uint16_t len;
};

static pci_device_t* dev = NULL;
static struct net_device* net_dev;
static uint8_t next_receive_page = 0;
static spinlock_t send_lock;

// Driver has only been tested with QEMU's emulation of the RTL8029AS so far
static const uint32_t vendor_device_combos[][2] = {
	{0x10ec, 0x8029}, // RealTek RTL8029
	{0x1050, 0x0940}, // Winbond 89C940
	{0x11f6, 0x1401}, // Compex RL2000
	{0x8e2e, 0x3000}, // KTI ET32P2
	{0x4a14, 0x5000}, // NetVin NV5000SC
	{0x1106, 0x0926}, // Via 86C926
	{0x10bd, 0x0e34}, // SureCom NE34
	{0x1050, 0x5a5a}, // Winbond W89C940F
	{0x12c3, 0x0058}, // Holtek HT80232
	{0x12c3, 0x5598}, // Holtek HT80229
	{0x8c4a, 0x1980}, // Winbond 89C940 8c4a
	{(uint32_t)NULL}
};

static inline void set_dma_la(int addr, size_t len) {
    ioutb(R_CR, CR_START | CR_DMA_NO);
	ioutb(R_RBCR0, len & 0xff);
	ioutb(R_RBCR1, len >> 8);
	ioutb(R_RSAR0, addr & 0xff);
	ioutb(R_RSAR1, addr >> 8);
}

static void pdma_write(int addr, const void* data, size_t len) {
	set_dma_la(addr, len);
	ioutb(R_CR, CR_START | CR_DMA_WRITE);

	uint8_t *p = (uint8_t*)data;
	for(size_t i = 0; i < len; i++) {
		ioutb(0x10, p[i]);
	}
}

static void pdma_read(int addr, void* data, size_t len) {
	set_dma_la(addr, len);
	ioutb(R_CR, CR_START | CR_DMA_READ);

	uint8_t *p = (uint8_t*)data;
	for(size_t i = 0; i < len; i++) {
		p[i] = iinb(0x10);
	}
}

static void receive() {
	// Mask receipt interrupts
	ioutb(R_IMR, 0x3a);

	ioutb(R_CR, CR_PAGE1);
	uint8_t curr = iinb(R_CURR);
	ioutb(R_CR, 0);

	// Clear intr
	ioutb(R_ISR, 0x1);

	while(next_receive_page != curr) {
		uint32_t index = next_receive_page * 0x100;

		// Get packet header
		struct recv_frame_header hdr;
		pdma_read(index, &hdr, sizeof(hdr));

		void* buf = kmalloc(hdr.len);
		pdma_read(index + 4, buf, hdr.len);
		net_receive(net_dev, buf, hdr.len);
		kfree(buf);

		next_receive_page = hdr.next;
	}

	ioutb(R_BNRY, curr - 1);
	ioutb(R_IMR, 0x3f);
}

static int send(struct pico_device* pdev, void* data, int len) {
	if(unlikely(!spinlock_get(&send_lock, 200))) {
		return -1;
	}

	pdma_write(MEM_TRANSMIT, data, len);
	ioutb(R_TBCR0, len & 0xff);
	ioutb(R_TBCR1, len >> 8);
	ioutb(R_CR, CR_START | CR_DMA_ABORT | CR_TRANSMIT);
	return len;
}

static void int_handler(task_t* task, isf_t* state, int num) {
	uint8_t isr = iinb(R_ISR);

	if(bit_get(isr, 0)) {
		if(unlikely(bit_get(isr, 2))) {
			log(LOG_ERR, "ne2k: Packet receive error\n");
		} else {
			receive();
		}
	}

	if(bit_get(isr, 1)) {
		if(unlikely(bit_get(isr, 3))) {
			log(LOG_ERR, "ne2k: Packet transmit error\n");
		}

		spinlock_release(&send_lock);
	}

	if(unlikely(bit_get(isr, 4))) {
		log(LOG_ERR, "ne2k: Overwrite warning\n");
	}

	if(unlikely(bit_get(isr, 5))) {
		log(LOG_ERR, "ne2k: Counter overflow!\n");
	}

	ioutb(R_ISR, isr &~ 1);
}

static int pci_cb(pci_device_t* _dev) {
	if(pci_check_vendor(_dev, vendor_device_combos) != 0) {
		return 1;
	}

	// FIXME Support multiple devices
	dev = _dev;
	log(LOG_INFO, "ne2k: Discovered device %p\n", dev);

	// Reset
	ioutb(0x1F, iinb(0x1F));
	while ((iinb(R_ISR) & 0x80) == 0);

	ioutb(R_CR, CR_STOP | CR_DMA_ABORT);

	// Set up interrupts and access length, reset counters
	ioutb(R_ISR, 0xff);
	ioutb(R_IMR, 0);
	ioutb(R_DCR, 0x48);
	ioutb(R_RBCR0, 0);
	ioutb(R_RBCR1, 0);

	// We want broadcast, multicast, and loopback mode.
	ioutb(R_RCR, (1 << 2) | (1 << 3));
	ioutb(R_TCR, 0x02);

	ioutb(R_PSTART, PAGE_RECEIVE);
	ioutb(R_BNRY, PAGE_RECEIVE);
	ioutb(R_PSTOP, PAGE_END);
	ioutb(R_TPSR, PAGE_TRANSMIT);
	next_receive_page = PAGE_RECEIVE + 1;

	// Figure out MAC address
	uint8_t mac[13];
	pdma_read(0, mac, 13);

	// Remove dup fields from mac - should fix pdma_read
	// Also set destination filter to our MAC address
	ioutb(R_CR, CR_STOP | CR_PAGE1);
	uint8_t nmac[6];
	for(int i = 0; i < 6; i++) {
		nmac[i] = mac[i*2];
		ioutb(0x01 + i, nmac[i]);
	}

	// Do this here since we're in page 1 anyway
	ioutb(R_CURR, PAGE_RECEIVE + 1);
	int_register(IRQ(dev->interrupt_line), int_handler, false);

	ioutb(R_CR, CR_STOP);	// Reset page
	ioutb(R_IMR, 0xff);		// Unmask all interrupts
	ioutb(R_CR, CR_START);

	net_dev = net_add_device("ne2k", nmac, send);

	return 0;
}

void ne2k_init() {
	pci_walk(pci_cb);
}

#endif /* ENABLE_NE2K */
