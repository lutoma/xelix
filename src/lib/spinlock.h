#pragma once

/* Copyright © 2011 Lukas Martini
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

#define SPINLOCK_LOCKED 1
#define SPINLOCK_UNLOCKED 0

#define spinlock_cmd(command, tries, retval) \
	static spinlock_t lock = SPINLOCK_UNLOCKED; \
	if(!spinlock_get(&lock, tries)) return retval; \
	do {command;} while(0); \
	spinlock_release(&lock);

typedef uint8_t spinlock_t;
uint32_t spinlock_get(spinlock_t* lock, uint32_t numretries);
void spinlock_release(spinlock_t* lock);