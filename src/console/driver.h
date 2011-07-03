#pragma once

/* Copyright © 2011 Fritz Grimpen
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

#include <lib/generic.h>
#include <console/info.h>

#define CONSOLE_DRV_CAP_CLEAR 0x01
#define CONSOLE_DRV_CAP_SCROLL  0x02
#define CONSOLE_DRV_CAP_SET_CURSOR 0x04

typedef struct {
	int (*write)(console_info_t*, char);
	char (*read)(console_info_t*);

	int capabilities;

	int (*_clear)(console_info_t *);
	int (*scroll)(console_info_t *, int32_t);
	void (*setCursor)(console_info_t *, uint32_t, uint32_t);
} console_driver_t;
