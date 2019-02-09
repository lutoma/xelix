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
#include <pico_dhcp_client.h>
#include <net/pico_dev.h>

static uint32_t dhcp_xid;
static bool initialized = false;

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

void net_init() {
	pico_stack_init();
	struct pico_device* dev = pico_xelix_create("ne2k1");
	if(!dev) {
		return;
	}

	pico_dhcp_initiate_negotiation(dev, &dhcp_cb, &dhcp_xid);
	initialized = true;
}
