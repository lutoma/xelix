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

static inline uint64_t profile_read_rdtsc() {
	register uint32_t timer_low asm("eax");
	register uint32_t timer_high asm("edx");
	asm volatile("rdtsc;" : "=r" (timer_low), "=r" (timer_high));
	return timer_low | (uint64_t)timer_high << 32;
}

static inline uint64_t profile_start() {
	return profile_read_rdtsc();
}

static inline uint64_t profile_stop(uint64_t start) {
	return profile_read_rdtsc() - start;
}
