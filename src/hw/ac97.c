/* ac97.c: Driver for the AC97 sound chip
 * Copyright © 2015, 2016 Lukas Martini
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

#ifdef ENABLE_AC97

#include <hw/ac97.h>
#include <hw/pci.h>

#include <log.h>
#include <portio.h>
#include <hw/interrupts.h>
#include <mem/kmalloc.h>
#include <string.h>
#include <multiboot.h>
#include <fs/vfs.h>

// Number of buffers to cache. More buffers = more latency. Maximum is 31.
#define NUM_BUFFERS 32

// Maximum hardware supported length is 0xFFFE (*2 for stereo)
#define BUFFER_LENGTH 0xFFFE

#define PORT_NAM_RESET				0x0000
#define PORT_NAM_MASTER_VOLUME		0x0002
#define PORT_NAM_AUX_OUT_VOLUME		0x0004
#define PORT_NAM_MONO_VOLUME		0x0006
#define PORT_NAM_PC_BEEP			0x000A
#define PORT_NAM_PCM_VOLUME			0x0018
#define PORT_NAM_EXT_AUDIO_ID		0x0028
#define PORT_NAM_EXT_AUDIO_STC		0x002A
#define PORT_NAM_FRONT_SPLRATE		0x002C
#define PORT_NAM_LR_SPLRATE			0x0032
#define PORT_NAM_FRONT_DAC_RATE		0x002C
#define PORT_NABM_POBDBAR			0x0010
#define PORT_NABM_POLVI				0x0015
#define PORT_NABM_POCONTROL			0x001B
#define PORT_NABM_GLB_CTRL_STAT		0x0060
#define PORT_NABM_PISTATUS			0x0006
#define PORT_NABM_POSTATUS			0x16
#define PORT_NABM_MCSTATUS			0x0026
#define PORT_NABM_PICONTROL			0x000B
#define PORT_NABM_POCONTROL			0x001B
#define PORT_NABM_MCCONTROL			0x002B
#define PORT_NABM_POPICB			0x0018
#define PORT_NABM_GLOB_STA			0x0030
#define NABM_POCIV					0x0014

/* Status register flags */
#define AC97_X_SR_DCH   (1 << 0)  /* DMA controller halted */
#define AC97_X_SR_CELV  (1 << 1)  /* Current equals last valid */
#define AC97_X_SR_LVBCI (1 << 2)  /* Last valid buffer completion interrupt */
#define AC97_X_SR_BCIS  (1 << 3)  /* Buffer completion interrupt status */
#define AC97_X_SR_FIFOE (1 << 3)  /* FIFO error */

/* PCM out control register flags */
#define AC97_X_CR_RPBM  (1 << 0)  /* Run/pause bus master */
#define AC97_X_CR_RR    (1 << 1)  /* Reset registers */
#define AC97_X_CR_LVBIE (1 << 2)  /* Last valid buffer interrupt enable */
#define AC97_X_CR_FEIE  (1 << 3)  /* FIFO error interrupt enable */
#define AC97_X_CR_IOCE  (1 << 4)  /* Interrupt on completion enable */

static uint32_t vendor_device_combos[][2] = {
	{0x8086, 0x2415},
	{0x8086, 0x2425},
	{0x8086, 0x2445},
	{(uint32_t)NULL}
};

struct buf_desc
{
	void* buf;
	uint16_t len;
	uint16_t reserve :14;
	uint8_t bup:1;
	uint8_t ioc:1;
} __attribute__((packed));

static int cards = 0;
static void* data;

// Debugging function
static void dump_regs(struct ac97_card* card) {
	uint16_t poctrl = inb(card->nabmbar + PORT_NABM_POCONTROL);
	uint16_t sr = inw(card->nabmbar + PORT_NABM_POSTATUS);

	log(LOG_DEBUG, "poctrl IOCE: %d\n", bit_get(poctrl, 4));
	log(LOG_DEBUG, "poctrl FEIE: %d\n", bit_get(poctrl, 3));
	log(LOG_DEBUG, "poctrl LVBIE: %d\n", bit_get(poctrl, 2));
	log(LOG_DEBUG, "poctrl RPBM: %d\n", bit_get(poctrl, 0));

	log(LOG_DEBUG, "postatus DCH: %d\n", bit_get(sr, 0));
	log(LOG_DEBUG, "postatus CELV: %d\n", bit_get(sr, 1));
	log(LOG_DEBUG, "postatus LVBCI: %d\n", bit_get(sr, 2));
	log(LOG_DEBUG, "postatus BCIS: %d\n", bit_get(sr, 3));
	log(LOG_DEBUG, "postatus FIFOE: %d\n", bit_get(sr, 4));

	uint8_t polvi = inb(card->nabmbar + PORT_NABM_POLVI);
	log(LOG_DEBUG, "POLVI: %d\n", polvi);

	/*uint16_t pcicmd = pci_config_read16(card->device, 0x04);
	log(LOG_DEBUG, "pcicmd MSE: %d\n", bit_get(pcicmd, 1));
	log(LOG_DEBUG, "pcicmd BME: %d\n", bit_get(pcicmd, 2));
	log(LOG_DEBUG, "pcicmd ID: %d\n", bit_get(pcicmd, 10));*/
}

static void fill_buffer(struct ac97_card* card, uint32_t offset, uint32_t bufno) {
	card->descs[bufno].buf = (void*)((intptr_t)data + (BUFFER_LENGTH * 2 * offset)); // FIXME This should do mapping to the phys addr, but kernel space is 1:1 mapped atm anyway.
	card->descs[bufno].len = BUFFER_LENGTH;
	card->descs[bufno].ioc = 1;
	card->descs[bufno].bup = 0;
}

static void interrupt_handler(isf_t *state) {
	struct ac97_card* card = &ac97_cards[0];

	/*// Find the card this IRQ is coming from
	for(int i = 0; i < cards; i++) {
		if(likely(state->interrupt == ac97_cards[i].device->interrupt_line + IRQ0)) {
			card = &ac97_cards[i];
		}
	}*/

	if(unlikely(card == NULL)) {
		log(LOG_ERR, "ac97: Could not locate card for interrupt. This shouldn't happen.\n");
		return;
	}

	uint16_t sr = inw(card->nabmbar + PORT_NABM_POSTATUS);

	if(sr & AC97_X_SR_LVBCI) {
		// Last valid buffer completion. Shouldn't actually happen
		outw(card->nabmbar + PORT_NABM_POSTATUS, AC97_X_SR_LVBCI);
	} else if(sr & AC97_X_SR_BCIS) {
		uint32_t current_buffer = (card->last_buffer + 1) % NUM_BUFFERS;
		uint16_t samples = inw(card->nabmbar + PORT_NABM_POPICB);

		//log(LOG_DEBUG, "ac97: Playing buffer %d, refilling buffer %d, %d samples left\n", current_buffer, card->last_buffer, samples);

		if(current_buffer == 0 || samples < 1) {
			outb(card->nabmbar + PORT_NABM_POLVI, NUM_BUFFERS);
		}

		outw(card->nabmbar + PORT_NABM_POSTATUS, AC97_X_SR_BCIS);

		// Refill the previous buffer
		fill_buffer(card, ++card->last_written_buffer, card->last_buffer);

		card->last_buffer = current_buffer;
	} else if(sr & AC97_X_SR_FIFOE) {
		outw(card->nabmbar + PORT_NABM_POSTATUS, AC97_X_SR_FIFOE);
	}
}

void ac97_set_volume(struct ac97_card* card, int volume) {
	// Set volume level of the various mixers
	// 0 = loud, 63 = almost silent
	outw(card->nambar + PORT_NAM_MASTER_VOLUME, (volume<<8) | volume);
	outw(card->nambar + PORT_NAM_AUX_OUT_VOLUME, (volume<<8) | volume);
	outw(card->nambar + PORT_NAM_MONO_VOLUME, volume);
	outw(card->nambar + PORT_NAM_PC_BEEP, volume);
	outw(card->nambar + PORT_NAM_PCM_VOLUME, (volume<<8) | volume);
}

static void set_sample_rate(struct ac97_card* card) {
	// Check if we can set the sample rate, and if so set it to 44.1 kHz
	if(inw(card->nambar + PORT_NAM_EXT_AUDIO_ID) & 1)
	{
		outw(card->nambar + PORT_NAM_EXT_AUDIO_STC, inw(card->nambar + PORT_NAM_EXT_AUDIO_STC) | 1);
		sleep_ticks(20);
		outw(card->nambar + PORT_NAM_FRONT_SPLRATE, 44100);
		outw(card->nambar + PORT_NAM_LR_SPLRATE, 44100);
		sleep_ticks(50);
	}

	// Read back & store actual sample rate
	card->sample_rate = inw(card->nambar + PORT_NAM_FRONT_DAC_RATE);
	log(LOG_DEBUG, "ac97: Sample rate set to %d Hz\n", card->sample_rate);
}

/* For some reason the whole kernel gets bricked when this is compiled with
 * optimizations on. Seems to be a bug around static functions. Blame GCC.
 */
static void __attribute__((optimize("O0"))) enable_card(struct ac97_card* card) {
	interrupts_register(card->device->interrupt_line + IRQ0, interrupt_handler, false);

	// Enable bus master, disable MSE
	//pci_config_write(card->device, 4, 5);

	sleep_ticks(30);

	// Get correct PCI bars for the sound chip control io ports
	card->nambar = pci_get_BAR(card->device, 0) & 0xFFFFFFFC;
	card->nabmbar = pci_get_BAR(card->device, 1) & 0xFFFFFFFC;

	// Turn power on / disable cold reset
	outl(card->nabmbar + 0x2c, bit_set(inl(card->nabmbar + 0x2c), 1));
	sleep_ticks(20);

	// Hard reset
	outw(card->nambar + PORT_NAM_RESET, 1);
	sleep_ticks(20);

	// Warm reset
	outl(card->nabmbar + 0x2c, bit_set(inl(card->nabmbar + 0x2c), 2));

	// Wait for reset to complete
	for(int i = 0;; i++) {
		if(bit_get(inl(card->nabmbar + 0x2c), 2) == 0) {
			log(LOG_INFO, "ac97: Warm reset finished.\n");
			break;
		}

		if(i > 60) {
			log(LOG_INFO, "ac97: Warm reset was not successful. Bailing.\n");
			return;
		}

		sleep_ticks(10);
	}

	sleep_ticks(20);

	ac97_set_volume(card, 10);
	sleep_ticks(20);

	set_sample_rate(card);
	card->descs = kmalloc_a(sizeof(struct buf_desc)*32);
	// FIXME This should do mapping to the phys addr, but kernel space is 1:1 mapped atm anyway.
	outl(card->nabmbar + PORT_NABM_POBDBAR, (intptr_t)card->descs);
}

void ac97_play(char* file) {
	struct ac97_card* card = &ac97_cards[0];
	int fd = vfs_open(file, O_RDONLY, NULL);
	if(fd == -1) {
		return;
	}

	// FIXME Use this properly, stat or use return value of vfs_read to choose buffer num
	data = kmalloc(9999999);
	size_t read = vfs_read(fd, data, 9999999, NULL);
	vfs_close(fd);

	if(read < 1) {
		return NULL;
	}

	for(int i = 0; i < NUM_BUFFERS; i++) {
		fill_buffer(card, i, i);
	}

	card->last_buffer = 0;
	card->last_written_buffer = NUM_BUFFERS - 1;

	outb(card->nabmbar + PORT_NABM_POLVI, NUM_BUFFERS);
	outb(card->nabmbar + PORT_NABM_POCONTROL, AC97_X_CR_RPBM | AC97_X_CR_FEIE | AC97_X_CR_IOCE);
}

void ac97_init()
{
	memset(ac97_cards, 0, AC97_MAX_CARDS * sizeof(struct ac97_card));

	pci_device_t** devices = (pci_device_t**)kmalloc(sizeof(pci_device_t*) * AC97_MAX_CARDS);
	uint32_t volatile num_devices = pci_search(devices, vendor_device_combos, AC97_MAX_CARDS);

	log(LOG_INFO, "ac97: Discovered %d devices.\n", num_devices);

	for(int i = 0; i < num_devices; ++i)
	{
		ac97_cards[i].device = devices[i];
		cards++;

		//devices[i]->interrupt_line = pci_get_interrupt_line(devices[i]);

		enable_card(&ac97_cards[i]);

		log(LOG_INFO, "ac97: %d:%d.%d, iobase 0x%x, irq %d\n",
				devices[i]->bus,
				devices[i]->dev,
				devices[i]->func,
				devices[i]->iobase,
				devices[i]->interrupt_line
			 );
	}
}

#endif /* ENABLE_AC97 */
