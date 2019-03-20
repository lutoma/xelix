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

#pragma once

static inline void outb(uint16_t port, uint8_t value) {
	asm volatile("outb %0, %1" : : "a" (value), "Nd" (port));
}

static inline void outw(uint16_t port, uint16_t value) {
	asm volatile("outw %0, %1" : : "a" (value), "Nd" (port));
}

static inline void outl(uint16_t port, uint32_t value) {
	asm volatile("outl %0, %1" : : "a" (value), "Nd" (port));
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile("inb %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

static inline uint16_t inw(uint16_t port) {
	uint16_t ret;
	asm volatile("inw %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

static inline uint32_t inl(uint16_t port) {
	uint32_t ret;
	asm volatile("inl %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}
