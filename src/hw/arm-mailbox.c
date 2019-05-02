/* arm-mailbox.c: Interface for BCM2836 VC mailboxes
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <portio.h>

#define ARMMBOX_BASE 0xB880

#define MBOX_READ   0x00
#define MBOX_WRITE  0x00
#define MBOX_POLL   0x10
#define MBOX_ID     0x14
#define MBOX_STATUS 0x18

#define MBOX0_BASE   ARMMBOX_BASE + 0x00
#define MBOX0_READ   MBOX0_BASE + MBOX_READ
#define MBOX0_WRITE  MBOX0_BASE + MBOX_WRITE
#define MBOX0_POLL   MBOX0_BASE + MBOX_POLL
#define MBOX0_ID     MBOX0_BASE + MBOX_ID
#define MBOX0_STATUS MBOX0_BASE + MBOX_STATUS
#define MBOX0_CFG    MBOX0_BASE + MBOX_READ

#define MBOX1_BASE   ARMMBOX_BASE + 0x20
#define MBOX1_READ   MBOX1_BASE + MBOX_READ
#define MBOX1_WRITE  MBOX1_BASE + MBOX_WRITE
#define MBOX1_POLL   MBOX1_BASE + MBOX_POLL
#define MBOX1_ID     MBOX1_BASE + MBOX_ID
#define MBOX1_STATUS MBOX1_BASE + MBOX_STATUS
#define MBOX1_CFG    MBOX1_BASE + MBOX_READ

#define MBOX_STATUS_FULL  (1 << 31)
#define MBOX_STATUS_EMPTY (1 << 30)

#define MBOX_CHAN(chan) ((chan) & 15)
#define MBOX_DATA(data) ((data) & ~15)
#define MBOX_MSG(data, chan) (MBOX_DATA(data) | MBOX_CHAN(chan))

uint32_t vc_mbox_read(unsigned chan) {
	while(1) {
		while(rpi_mmio_read(MBOX0_STATUS) & MBOX_STATUS_EMPTY);

		uint32_t val = rpi_mmio_read(MBOX0_READ);
		if (MBOX_CHAN(val) == chan)
			return MBOX_DATA(val);
	}
}

void vc_mbox_write(uint32_t msg, unsigned chan) {
	while(rpi_mmio_read(MBOX0_STATUS) & MBOX_STATUS_FULL);
	rpi_mmio_write(MBOX1_WRITE, MBOX_MSG(msg, chan));
}
