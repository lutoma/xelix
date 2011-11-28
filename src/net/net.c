/* net.c: Network organization
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

#include "net.h"

#include <lib/log.h>
#include <tasks/scheduler.h>
#include <memory/kmalloc.h>

#define MAX_DEVICES 51
net_device_t registered_devices[MAX_DEVICES];
uint32_t registered_device_count;

void net_receive(net_device_t* origin, int target, void* data)
{
	log(LOG_DEBUG, "net: net_receive: Incoming packet from %s with target %d.\n", origin->name, target);
}

void net_register_device(net_device_t* device)
{
	registered_devices[registered_device_count++] = *device;
	kfree(device);
}
