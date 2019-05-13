#pragma once

/* Copyright © 2011-2019 Lukas Martini
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

#include <stdbool.h>

/* See https://gcc.gnu.org/onlinedocs/gcc-4.4.3/gcc/Atomic-Builtins.html for
 * documentation on the GCC builtin atomic function used here.
 */

#define spinlock_release(x) __sync_lock_release(x)
#define spinlock_cmd(command, tries, retval) \
	static spinlock_t lock = 0; \
	if(!spinlock_get(&lock, tries)) return retval; \
	do {command;} while(0); \
	spinlock_release(&lock);

typedef uint8_t spinlock_t;
static inline bool spinlock_get(spinlock_t* lock, uint32_t retries) {
	for(int i = 0; i < retries; i++) {
		if(!__sync_lock_test_and_set(lock, 1)) {
			return true;
		}

		halt();
	}
	return false;
}

