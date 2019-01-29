/* gettimeofday.c: Get time
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

#include <tasks/syscall.h>
#include <lib/time.h>

struct timeval {
	uint32_t tv_sec;
	uint32_t tv_usec;
};

SYSCALL_HANDLER(gettimeofday) {
	SYSCALL_SAFE_RESOLVE_PARAM(0);
	struct timeval* tv = (struct timeval*)syscall.params[0];
	tv->tv_sec = time_get();
	tv->tv_usec = 0;
	return 0;
}
