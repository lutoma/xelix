/* virtio-net.c: Driver for virtio networking
 * Copyright © 2015 Lukas Martini
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

/* The relevant virtio spec can be found at:
 * http://docs.oasis-open.org/virtio/virtio/v1.0/csprd01/virtio-v1.0-csprd01.html
 */

#include <hw/virtio.h>
#include <hw/virtio-net.h>
#include <hw/pci.h>

#include <lib/generic.h>
#include <lib/log.h>
#include <lib/print.h>
#include <lib/portio.h>
#include <interrupts/interface.h>
#include <memory/kmalloc.h>
#include <net/ether.h>
#include <net/net.h>
#include <lib/string.h>

// TODO Check revision id of device for 1 (non-transitional) (instead of 0, transitional) at init

// FIXME: PCI needs range detection
static uint32_t vendor_device_combos[][2] = {
	{0x1AF4, 0x1000}, {0x1AF4, 0x1001}, {0x1AF4, 0x1002},
	{0x1AF4, 0x1003}, {0x1AF4, 0x1004}, {0x1AF4, 0x1005},
	{0x1AF4, 0x1006}, {0x1AF4, 0x1007}, {0x1AF4, 0x1008},
	{0x1AF4, 0x1009}, {0x1AF4, 0x100a}, {0x1AF4, 0x100b},
	{0x1AF4, 0x100c}, {0x1AF4, 0x100d}, {0x1AF4, 0x100e},
	{0x1AF4, 0x100f}, {0x1AF4, 0x1010}, {0x1AF4, 0x1011},
	{0x1AF4, 0x1012}, {0x1AF4, 0x1013}, {0x1AF4, 0x1014},
	{0x1AF4, 0x1015}, {0x1AF4, 0x1016}, {0x1AF4, 0x1017},
	{0x1AF4, 0x1018}, {0x1AF4, 0x1019}, {0x1AF4, 0x101a},
	{0x1AF4, 0x101b}, {0x1AF4, 0x101c}, {0x1AF4, 0x101d},
	{0x1AF4, 0x101e}, {0x1AF4, 0x101f}, {0x1AF4, 0x1020},
	{0x1AF4, 0x1021}, {0x1AF4, 0x1022}, {0x1AF4, 0x1023},
	{0x1AF4, 0x1024}, {0x1AF4, 0x1025}, {0x1AF4, 0x1026},
	{0x1AF4, 0x1027}, {0x1AF4, 0x1028}, {0x1AF4, 0x1029},
	{0x1AF4, 0x102a}, {0x1AF4, 0x102b}, {0x1AF4, 0x102c},
	{0x1AF4, 0x102d}, {0x1AF4, 0x102e}, {0x1AF4, 0x102f},
	{0x1AF4, 0x1030}, {0x1AF4, 0x1031}, {0x1AF4, 0x1032},
	{0x1AF4, 0x1033}, {0x1AF4, 0x1034}, {0x1AF4, 0x1035},
	{0x1AF4, 0x1036}, {0x1AF4, 0x1037}, {0x1AF4, 0x1038},
	{0x1AF4, 0x1039}, {0x1AF4, 0x103a}, {0x1AF4, 0x103b},
	{0x1AF4, 0x103c}, {0x1AF4, 0x103d}, {0x1AF4, 0x103e},
	{0x1AF4, 0x103f}, {(uint32_t)NULL}
};

// This driver will only support that many cards
#define MAX_CARDS 50 

// The virtio-net extensions this driver supports
#define DRIVER_SUPPORTED_EXTENSIONS VIRTIO_NET_F_MAC

#define	VIRTIO_NET_CTRL_RX		0
#define	VIRTIO_NET_CTRL_RX_PROMISC	0
#define	VIRTIO_NET_CTRL_RX_ALLMULTI	1

#define	VIRTIO_NET_CTRL_MAC		1
#define	VIRTIO_NET_CTRL_MAC_TABLE_SET	0

#define	VIRTIO_NET_CTRL_VLAN		2
#define	VIRTIO_NET_CTRL_VLAN_ADD	0
#define	VIRTIO_NET_CTRL_VLAN_DEL	1

#define	VIRTIO_NET_S_LINK_UP	1

// FIXME Those are not virtio_net specific. Move to virtio.h.
#define VIRTIO_NET_IO_DEV_FEATURE 0
#define VIRTIO_NET_IO_DRV_FEATURE 4
#define VIRTIO_NET_IO_STATUS 18
#define VIRTIO_NET_IO_QUEUE_SELECT 14
#define VIRTIO_NET_IO_QUEUE_SIZE 12
#define VIRTIO_NET_IO_QUEUE_PFN 8

#define VIRTIO_NET_PCI_STATUS_RESET 0x00
#define VIRTIO_NET_PCI_STATUS_ACKNOWLEDGE 0x01
#define VIRTIO_NET_PCI_STATUS_DRIVER 0x02
#define VIRTIO_NET_PCI_STATUS_DRIVER_OK 0x04
#define VIRTIO_NET_PCI_STATUS_FEATURES_OK 0x8
#define VIRTIO_NET_PCI_STATUS_FAILED 0x80

#define	VIRTIO_NET_F_CSUM	(1 << 0) /* Host handles pkts w/ partial csum */
#define	VIRTIO_NET_F_GUEST_CSUM	(1 << 1) /* Guest handles pkts w/ part csum */
#define	VIRTIO_NET_F_MAC	(1 << 5) /* Host has given MAC address. */
#define	VIRTIO_NET_F_GSO	(1 << 6) /* Host handles pkts w/ any GSO type */
#define	VIRTIO_NET_F_GUEST_TSO4	(1 << 7) /* Guest can handle TSOv4 in. */
#define	VIRTIO_NET_F_GUEST_TSO6	(1 << 8) /* Guest can handle TSOv6 in. */
#define	VIRTIO_NET_F_GUEST_ECN	(1 << 9) /* Guest can handle TSO[6] w/ ECN in */
#define	VIRTIO_NET_F_GUEST_UFO	(1 << 10) /* Guest can handle UFO in. */
#define	VIRTIO_NET_F_HOST_TSO4	(1 << 11) /* Host can handle TSOv4 in. */
#define	VIRTIO_NET_F_HOST_TSO6	(1 << 12) /* Host can handle TSOv6 in. */
#define	VIRTIO_NET_F_HOST_ECN	(1 << 13) /* Host can handle TSO[6] w/ ECN in */
#define	VIRTIO_NET_F_HOST_UFO	(1 << 14) /* Host can handle UFO in. */
#define	VIRTIO_NET_F_MRG_RXBUF	(1 << 15) /* Host can merge receive buffers. */
#define	VIRTIO_NET_F_STATUS	(1 << 16) /* Config.status available */
#define	VIRTIO_NET_F_CTRL_VQ	(1 << 17) /* Control channel available */
#define	VIRTIO_NET_F_CTRL_RX	(1 << 18) /* Control channel RX mode support */
#define	VIRTIO_NET_F_CTRL_VLAN	(1 << 19) /* Control channel VLAN filtering */
#define	VIRTIO_NET_F_CTRL_RX_EXTRA (1 << 20) /* Extra RX mode control support */

#define int_out8(card, port, value) portio_out8(card ->device->iobase + port, value)
#define int_out16(card, port, value) portio_out16(card ->device->iobase + port, value)
#define int_out32(card, port, value) portio_out32(card ->device->iobase + port, value)

#define int_in8(card, port) portio_in8(card ->device->iobase + port)
#define int_in16(card, port) portio_in16(card->device->iobase + port)
#define int_in32(card, port) portio_in32(card->device->iobase + port)

struct virtio_net_card {
	pci_device_t *device;
	uint32_t features;
	char mac_address[6];
};

// Packet header structure
struct virtio_net_hdr {
	uint8_t		flags;
	uint8_t		gso_type;
	uint16_t	hdr_len;
	uint16_t	gso_size;
	uint16_t	csum_start;
	uint16_t	csum_offset;
};

static char* feature_flags_verbose[] = {
	"Host CSUM",
	"Guest CSUM",
	"2", "3", "4",
	"MAC",
	"GSO",
	"TSOv4",
	"TSOv6",
	"TSO[6] + ECN",
	"UFO",
	"merge bfs",
	"Config.status",
	"ctrl chan",
	"ctrl chan RX",
	"VLAN",
	"RX mode control"
};

static int cards = 0;
static struct virtio_net_card virtio_net_cards[MAX_CARDS];

// FIXME Properly implement "2.3 Conﬁguration Space" from virtio spec

static void virtio_net_int_handler(cpu_state_t *state)
{
	log(LOG_DEBUG, "virtio-net: Got interrupt.\n");
}

static void load_mac(struct virtio_net_card* card) {
	memset(&card->mac_address, 0, 6);

	if(!(card->features & VIRTIO_NET_F_MAC)) {
		/* Host doesn't support MAC address setting / reading, so just leave it
		 * all-zeroes for internal use.
		 */
		log(LOG_WARN, "virtio_net: Host does not support MAC addresses\n");
		return;
	}

	for(int i = 0; i < 16; i++) {
		card->mac_address[i] = int_in8(card, 0x14 + i);
	}
}

static inline void _print_features(char* type, uint32_t features) {
	log(LOG_DEBUG, "virtio_net: %s features (0x%x): ", type, features);
	for(int i = 0; i < 16; i++) {
		if(bit_get(features, i)) {
			// FIXME should use log
			printf("%s ", feature_flags_verbose[i]);
		}
	}
	printf("\n");
}

static bool negotiate_features(struct virtio_net_card* card) {
	// Get host supported features
	uint32_t host_features = int_in32(card, VIRTIO_NET_IO_DEV_FEATURE);
	_print_features("Host supported", host_features);

	// Find out what extensions both we (only MAC so far) and the card support
	uint32_t features_consensus = host_features & DRIVER_SUPPORTED_EXTENSIONS;
	
	// Send our proposed feature consensus to card
	// TODO What about VIRTIO_NET_IO_DRV_FEATURE_SELECT? Probably not relevant for _net
	int_out32(card, VIRTIO_NET_IO_DRV_FEATURE, features_consensus);
	int_out8(card, VIRTIO_NET_IO_STATUS, VIRTIO_NET_PCI_STATUS_FEATURES_OK);

	// Check if card is happy
	sleep_ticks(10);
	if(int_in8(card, VIRTIO_NET_IO_STATUS) != VIRTIO_NET_PCI_STATUS_FEATURES_OK) {
		log(LOG_ERR, "virtio_net: Feature negotiation failed (card reject)\n");
		return false;
	}

	card->features = features_consensus;
	_print_features("Active", features_consensus);

	return true;
}

static bool setup_virtqueue(struct virtio_net_card *card, uint8_t queue_id) {
	log(LOG_DEBUG, "Initializing virtqueue %d\n", queue_id);
	int_out16(card, VIRTIO_NET_IO_QUEUE_SELECT, queue_id);

	// Check if this vq is actually supported by the card
	uint16_t queue_size = int_in16(card, VIRTIO_NET_IO_QUEUE_SIZE);
	if(queue_size < 1) {
		log(LOG_ERR, "virtio_net: Attempt to set up unsupported virtqueue %d\n", queue_id);
	}

	log(LOG_INFO, "virtio_net: Queue %d length of %d\n", queue_id, queue_size);

	// FIXME Should these two actually be separate?
	struct vring* vring = kmalloc_a(sizeof(struct vring));
	
	int vring_bsize = vring_size(queue_size, 4096);
	void* vring_data = kmalloc_a(vring_bsize);
	memset(vring_data, 0, vring_bsize);

	vring_init(vring, queue_size, vring_data, 4096);

	int_out32(card, VIRTIO_NET_IO_QUEUE_PFN, vring);
}

static bool enable_card(struct virtio_net_card *card)
{
	// Reset the device in case it was left in a weird state somehow
	int_out8(card, VIRTIO_NET_IO_STATUS, VIRTIO_NET_PCI_STATUS_RESET);

	// Check if the reset was sucessful (device should have status of 0)
	if(int_in8(card, VIRTIO_NET_IO_STATUS) != 0) {
		log(LOG_ERR, "virtio-net: Device reset was not successful.\n");
		return false;
	}

	// Acknowledge, set us as driver
	// FIXME, This is actually a bitmap, we should use it accordingly instead
	// of always overwriting the whole thing
	int_out8(card, VIRTIO_NET_IO_STATUS, VIRTIO_NET_PCI_STATUS_ACKNOWLEDGE);
	int_out8(card, VIRTIO_NET_IO_STATUS, VIRTIO_NET_PCI_STATUS_DRIVER);

	negotiate_features(card);
	load_mac(card);

	// Get receive, transmit & control queues
	setup_virtqueue(card, 0);
	interrupts_registerHandler(card->device->interruptLine + IRQ0, virtio_net_int_handler);
	int_out8(card, VIRTIO_NET_IO_STATUS, VIRTIO_NET_PCI_STATUS_DRIVER_OK);

	return true;
}

void virtio_net_init()
{
	memset(virtio_net_cards, 0, MAX_CARDS * sizeof(struct virtio_net_card));

	pci_device_t** devices = (pci_device_t**)kmalloc(sizeof(void*) * MAX_CARDS);
	uint32_t num_devices = pci_search_by_id(devices, vendor_device_combos, MAX_CARDS);
	
	int i;
	for(i = 0; i < num_devices; ++i)
	{
		// Check if this is actually a network chip
		if(devices[i]->subsystemID != PCI_PRODUCT_VIRTIO_NETWORK) {
			log(LOG_INFO, "virtio_net: Detected virtio device is not a network bridge, bailing.\n");
			continue;
		}

		virtio_net_cards[i].device = devices[i];
		cards++;

		if(!enable_card(&virtio_net_cards[i])) {
			/* Initialization failed for some reason. Remove the card here and
			 * tell the host system about the initialization failure.
			 */
			portio_out8(virtio_net_cards[i].device->iobase + VIRTIO_NET_IO_STATUS,
				VIRTIO_NET_PCI_STATUS_FAILED);

			virtio_net_cards[i].device = NULL;
			cards--;
			continue;
		}

		log(LOG_INFO, "virtio_net: %d:%d.%d, iobase 0x%x, irq %d, MAC Address %x:%x:%x:%x:%x:%x\n",
				devices[i]->bus,
				devices[i]->dev,
				devices[i]->func,
				devices[i]->iobase,
				devices[i]->interruptLine,
				virtio_net_cards[i].mac_address[0],
				virtio_net_cards[i].mac_address[1],
				virtio_net_cards[i].mac_address[2],
				virtio_net_cards[i].mac_address[3],
				virtio_net_cards[i].mac_address[4],
				virtio_net_cards[i].mac_address[5]
			 );
	}

	log(LOG_INFO, "virtio_net: Discovered %d device%p.\n", num_devices);
}
