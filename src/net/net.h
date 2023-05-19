#pragma once

/* Copyright © 2019 Lukas Martini
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

#include <pico_device.h>
#include <buffer.h>
#include <spinlock.h>

struct net_device {
	struct pico_device pico_dev;
	struct buffer* recv_buf;
};

typedef int (net_send_callback_t)(struct pico_device* pico_dev, void* data, int size);
extern spinlock_t net_pico_lock;

void net_receive(struct net_device* dev, void* data, size_t len);
struct net_device* net_add_device(char* name, uint8_t mac[6], net_send_callback_t* write_cb);
void net_init();
