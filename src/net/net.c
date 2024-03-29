/* net.c: PicoTCP integration
 * Copyright © 2019-2020 Lukas Martini
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "net.h"
#include <buffer.h>
#include <spinlock.h>
#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_device.h>
#include <pico_dhcp_client.h>
#include <pico_dns_client.h>
#include <pico_dev_loop.h>
#include <net/i386-rtl8139.h>
#include <net/i386-ne2k.h>
#include <net/virtio_net.h>
#include <tasks/worker.h>
#include <tasks/scheduler.h>
#include <time.h>

#ifdef CONFIG_ENABLE_PICOTCP

spinlock_t net_pico_lock;
static bool initialized = false;
static uint32_t dhcp_xid;

static void dhcp_cb(void* cli, int code) {
	if(code & PICO_DHCP_ERROR) {
		log(LOG_INFO, "net: DHCP failed.\n");
		return;
	}

	struct pico_ip4 ipaddr = pico_dhcp_get_address(cli);
	char ip[16];
	pico_ipv4_to_string(ip, ipaddr.addr);
	log(LOG_INFO, "net: DHCP done, IP %s\n", ip);
}

static int pico_dsr_cb(struct pico_device* pico_dev, int loop_score) {
	struct net_device* dev = (struct net_device*)pico_dev;

	size_t sz = buffer_size(dev->recv_buf);
	if(likely(sz)) {
		void* buf = kmalloc(sz);
		buffer_pop(dev->recv_buf, buf, sz);
		pico_stack_recv_zerocopy(pico_dev, buf, sz);
		loop_score--;
	}

	pico_dev->__serving_interrupt = 0;
	return loop_score;
}

// Receive data from device
void net_receive(struct net_device* dev, void* data, size_t len) {
	if(unlikely(!initialized)) {
		return;
	}

	if(buffer_write(dev->recv_buf, data, len) < len) {
		log(LOG_WARN, "net: Receive buffer overflow, discarding incoming packets\n");
	}

	dev->pico_dev.__serving_interrupt = 1;
}

struct net_device* net_add_device(char* name, uint8_t mac[6], net_send_callback_t* send_cb) {
	struct net_device* dev = zmalloc(sizeof(struct net_device));
	struct pico_ethdev* eth = zmalloc(sizeof(struct pico_ethdev));
	if(!dev || !eth) {
		return NULL;
	}

	if(pico_device_init(&dev->pico_dev, name, NULL)) {
		return NULL;
	}

	memcpy(eth->mac.addr, mac, sizeof(uint8_t) * 6);
	dev->recv_buf = buffer_new(40);
	if(!dev->recv_buf) {
		return NULL;
	}

	dev->pico_dev.eth = eth;
	dev->pico_dev.send = send_cb;
	dev->pico_dev.dsr = pico_dsr_cb;

	log(LOG_INFO, "net: New device %s mac %02x:%02x:%02x:%02x:%02x:%02x\n",
		name, mac[0], mac[1], mac[2], mac[3],
		mac[4], mac[5]);

	pico_dhcp_initiate_negotiation(&dev->pico_dev, &dhcp_cb, &dhcp_xid);
	return dev;
}

static void __attribute__((fastcall, noreturn)) net_worker_entry(worker_t* worker) {
	while(1) {
		if(likely(initialized)) {
			pico_stack_tick();
		}
		scheduler_yield();
	}
}

void net_init() {
	log(LOG_INFO, "net: Initializing PicoTCP\n");
	pico_stack_init();
	initialized = true;

	uint32_t ilo_addr;
	pico_string_to_ipv4("127.0.0.1", &ilo_addr);
	struct pico_ip4 lo_addr = {.addr = ilo_addr};

	uint32_t inetmask;
	pico_string_to_ipv4("255.0.0.0", &inetmask);
	struct pico_ip4 netmask = {.addr = inetmask};

	uint32_t isubnet;
	pico_string_to_ipv4("127.0.0.0", &isubnet);
	struct pico_ip4 subnet = {.addr = isubnet};

	struct pico_device* lo = pico_loop_create();
	pico_ipv4_link_add(lo, lo_addr, netmask);
	pico_ipv4_route_add(subnet, netmask, lo_addr, 1000, NULL);

	log(LOG_INFO, "net: Loading device drivers\n");

	#ifdef CONFIG_ENABLE_VIRTIO_NET
	virtio_net_init();
	#endif

	#ifdef CONFIG_ENABLE_NE2K
	ne2k_init();
	#endif

	#ifdef CONFIG_ENABLE_RTL8139
	rtl8139_init();
	#endif

	worker_t* net_worker = worker_new("knetworkd", net_worker_entry);
	scheduler_add_worker(net_worker);
}

#endif /* ENABLE_PICOTCP */
