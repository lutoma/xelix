#pragma once

/* Copyright © 2018 Lukas Martini
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

#include <bsp/timer.h>

typedef uint32_t time_t;
struct timeval {
	uint64_t tv_sec;
	uint64_t tv_usec;
};

uint32_t time_get();
int time_get_timeval(struct timeval* tv);
void time_init();

#define sleep(t) sleep_ticks((t) * timer_rate)
static inline void __attribute__((optimize("O0"))) sleep_ticks(time_t timeout) {
	const uint32_t until = timer_tick + timeout;
	while(timer_tick <= until) {
		halt();
	}
}

#define uptime() (timer_tick / timer_rate)
