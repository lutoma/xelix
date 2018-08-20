/* spinlock.c: Simple spinlocks using GCC's atomic builtins
 * Copyright © 2015 Lukas Martini
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

#include <generic.h>
#include <spinlock.h>
#include <tasks/scheduler.h>

/* See https://gcc.gnu.org/onlinedocs/gcc-4.4.3/gcc/Atomic-Builtins.html for
 * documentation on the GCC atomic builtin function used here.
 */

bool spinlock_get(spinlock_t* lock, uint32_t numretries) {
	for(int i = 0; i < numretries; i++) {
		if(__sync_bool_compare_and_swap(lock, 0, 1)) {
			return true;
		}

		scheduler_yield();
	}

	return false;
}

