/* net.c: PicoTCP integration
 * Copyright © 2019 Lukas Martini
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
#include <pico_stack.h>
#include <pico_ipv4.h>
#include <pico_device.h>
#include <pico_dhcp_client.h>
#include <spinlock.h>

#define RECV_BUFFER_SIZE 2048

static bool initialized = false;
static uint32_t dhcp_xid;

void dhcp_cb(void* cli, int code) {
	if(code & PICO_DHCP_ERROR) {
		log(LOG_INFO, "net: DHCP failed.\n");
		return;
	}

	struct pico_ip4 ipaddr = pico_dhcp_get_address(cli);
	char ip[16];
	pico_ipv4_to_string(ip, ipaddr.addr);
	log(LOG_INFO, "net: DHCP done, IP %s\n", ip);
}

void net_tick() {
	if(!initialized) {
		return;
	}

	pico_stack_tick();
}

static int pico_dsr_cb(struct pico_device* pico_dev, int loop_score) {
	struct net_device* dev = (struct net_device*)pico_dev;
	if(!spinlock_get(&dev->recv_buf_lock, 200)) {
		return loop_score;
	}

	if(dev->recv_buf_len) {
		pico_stack_recv(pico_dev, dev->recv_buf, dev->recv_buf_len);
		dev->recv_buf_len = 0;
		loop_score--;
	}

	spinlock_release(&dev->recv_buf_lock);
	return loop_score;
}

// Receive data from device
void net_receive(struct net_device* dev, void* data, size_t len) {
	if(dev->recv_buf_len + len > RECV_BUFFER_SIZE) {
		log(LOG_WARN, "net: Receive buffer overflow, discarding incoming packets\n");
		return;
	}

	if(!spinlock_get(&dev->recv_buf_lock, 200)) {
		return;
	}

	memcpy(dev->recv_buf + dev->recv_buf_len, data, len);
	dev->recv_buf_len += len;
	dev->pico_dev.__serving_interrupt = 1;
	spinlock_release(&dev->recv_buf_lock);
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

	memcpy(eth->mac.addr, mac, sizeof(uint8_t) + 6);
	dev->recv_buf = zmalloc(RECV_BUFFER_SIZE);
	dev->pico_dev.eth = eth;
	dev->pico_dev.send = send_cb;
	dev->pico_dev.dsr = pico_dsr_cb;

	log(LOG_INFO, "net: New device %s mac %02x:%02x:%02x:%02x:%02x:%02x\n",
		name, mac[0], mac[1], mac[2], mac[3],
		mac[4], mac[5], mac[6]);

	pico_dhcp_initiate_negotiation(&dev->pico_dev, &dhcp_cb, &dhcp_xid);
	return dev;
}

void net_init() {
	pico_stack_init();
	initialized = true;
}
