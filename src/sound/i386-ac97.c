/* ac97.c: Driver for the AC97 sound chip
 * Copyright © 2015-2020 Lukas Martini
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

#ifdef CONFIG_ENABLE_AC97

#include <sound/i386-ac97.h>
#include <log.h>
#include <portio.h>
#include <string.h>
#include <stdbool.h>
#include <int/int.h>
#include <bsp/i386-pci.h>
#include <mem/kmalloc.h>
#include <mem/mem.h>
#include <boot/multiboot.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>

// Number of buffers to cache. More buffers = more latency. Maximum is 32.
#define NUM_BUFFERS 32
#define AC97_MAX_CARDS 1

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
#define PORT_NABM_POCIV				0x0014
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
	{0}
};

struct buf_desc {
	void* buf;
	uint16_t len;
	uint16_t _unused:14;
	uint8_t bup:1;
	uint8_t ioc:1;
} __attribute__((packed));

struct ac97_card {
	pci_device_t *device;
	uint16_t nambar;
	uint16_t nabmbar;
	uint16_t sample_rate;

	struct buf_desc* descs;
	void* buffers[NUM_BUFFERS];

	// Currently playing buffer, -1 if not playing anything
	int playing_buffer;

	// Number of next buf_desc to write to
	int buf_next_write;
};

static struct ac97_card ac97_cards[AC97_MAX_CARDS];
static int cards = 0;

static void interrupt_handler(task_t* task, isf_t* state, int num) {
	struct ac97_card* card = &ac97_cards[0];
	/*// Find the card this IRQ is coming from
	for(int i = 0; i < cards; i++) {
		if(likely(state->interrupt == IRQ(ac97_cards[i].device->interrupt_line))) {
			card = &ac97_cards[i];
		}
	}*/

	if(unlikely(!card)) {
		return;
	}

	uint16_t sr = inw(card->nabmbar + PORT_NABM_POSTATUS);
	if(sr & AC97_X_SR_LVBCI) {
		card->playing_buffer = -1;
		outw(card->nabmbar + PORT_NABM_POSTATUS, AC97_X_SR_LVBCI);
	} else if(sr & AC97_X_SR_BCIS) {
		if(card->playing_buffer != -1) {
			card->playing_buffer = inb(card->nabmbar + PORT_NABM_POCIV);
		}

		outw(card->nabmbar + PORT_NABM_POSTATUS, AC97_X_SR_BCIS);
	} else if(sr & AC97_X_SR_FIFOE) {
		card->playing_buffer = -1;
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
	if(inw(card->nambar + PORT_NAM_EXT_AUDIO_ID) & 1) {
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

static size_t sfs_write(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	struct ac97_card* card = &ac97_cards[0];

	int bno = card->buf_next_write;
	card->buf_next_write = (card->buf_next_write + 1) % NUM_BUFFERS;

	int_enable();
	while(bno == card->playing_buffer) {
		halt();
	}
	int_disable();

	struct buf_desc* desc = &card->descs[bno];
	size = MIN(size, PAGE_SIZE * 4);
	desc->len = size / 2;
	desc->ioc = 1;
	desc->bup = 0;
	memcpy(card->buffers[bno], source, size);
	outb(card->nabmbar + PORT_NABM_POLVI, bno);

	if(card->playing_buffer == -1) {
		card->playing_buffer = bno;
		outb(card->nabmbar + PORT_NABM_POCONTROL,
			AC97_X_CR_RPBM | AC97_X_CR_FEIE | AC97_X_CR_IOCE | AC97_X_CR_LVBIE);
	}

	return size;
}

static void enable_card(struct ac97_card* card) {
	int_register(IRQ(card->device->interrupt_line), interrupt_handler, false);

	// Enable bus master, disable MSE
	pci_config_write(card->device, 4, 5);
	sleep_ticks(30);

	// Get correct PCI bars for the sound chip control io ports
	card->nambar = pci_get_BAR(card->device, 0) & 0xFFFFFFFC;
	card->nabmbar = pci_get_BAR(card->device, 1) & 0xFFFFFFFC;
	card->playing_buffer = -1;

	// Turn power on / disable cold reset
	outl(card->nabmbar + 0x2c, inl(card->nabmbar + 0x2c) | 1 << 1);
	sleep_ticks(20);

	// Hard reset
	outw(card->nambar + PORT_NAM_RESET, 1);
	sleep_ticks(20);

	// Warm reset
	outl(card->nabmbar + 0x2c, inl(card->nabmbar + 0x2c) | 1 << 2);

	// Wait for reset to complete
	for(int i = 0;; i++) {
		if((inl(card->nabmbar + 0x2c) & (1 << 2)) == 0) {
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

	ac97_set_volume(card, 5);
	sleep_ticks(20);

	set_sample_rate(card);
	card->descs = zmalloc_a(sizeof(struct buf_desc) * NUM_BUFFERS);

	for(int i = 0; i < NUM_BUFFERS; i++) {
		void* phys = palloc(4);
		card->buffers[i] = valloc(4, phys, VM_RW);
		card->descs[i].buf = phys;
	}

	outl(card->nabmbar + PORT_NABM_POBDBAR, (uintptr_t)card->descs);

	struct vfs_callbacks sfs_cb = {
		.write = sfs_write,
	};
	sysfs_add_dev("dsp1", &sfs_cb);
}

void ac97_init() {
	memset(ac97_cards, 0, AC97_MAX_CARDS * sizeof(struct ac97_card));
	pci_device_t** devices = (pci_device_t**)kmalloc(sizeof(pci_device_t*) * AC97_MAX_CARDS);
	uint32_t num_devices = pci_search(devices, vendor_device_combos, AC97_MAX_CARDS);
	log(LOG_INFO, "ac97: Discovered %d devices.\n", num_devices);

	for(int i = 0; i < num_devices; ++i) {
		ac97_cards[i].device = devices[i];
		cards++;

		enable_card(&ac97_cards[i]);
		log(LOG_INFO, "ac97: %d:%d.%d, iobase 0x%x, irq %d\n",
			devices[i]->bus, devices[i]->dev, devices[i]->func,
			devices[i]->iobase, devices[i]->interrupt_line);
	}
}

#endif /* ENABLE_AC97 */
