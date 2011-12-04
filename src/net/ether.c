/* ether.c: Ethernet Layer
 * Copyright © 2011 Fritz Grimpen
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

#include "ether.h"
#include "net.h"

void net_ether_receive(net_device_t *dev, uint8_t *data, size_t len)
{
	ether_frame_hdr_t *hdr = (ether_frame_hdr_t*)data;

	int net_target = hdr->type;
	
	switch (hdr->type)
	{
		case 0x0008:
			net_target = NET_PROTO_IP4;
			break;
	}

	net_receive(dev, net_target, len - sizeof(ether_frame_hdr_t), data + sizeof(ether_frame_hdr_t));
}
