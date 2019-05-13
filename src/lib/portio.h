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

#ifdef __i386__
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
	uint8_t ret = 0;
	asm volatile("inb %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

static inline uint16_t inw(uint16_t port) {
	uint16_t ret = 0;
	asm volatile("inw %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

static inline uint32_t inl(uint16_t port) {
	uint32_t ret = 0;
	asm volatile("inl %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}
#endif

#ifdef __arm__
// for raspi2 & 3, 0x20000000 for raspi1
#define RPI_MMIO_BASE 0x3F000000
#define BCM2836_MMIO_BASE 0x40000000
#define rpi_mmio_write(reg, data) mmio_write(RPI_MMIO_BASE + (reg), data)
#define rpi_mmio_read(reg) mmio_read(RPI_MMIO_BASE + (reg))
#define bcm2836_mmio_write(reg, data) mmio_write(BCM2836_MMIO_BASE + (reg), data)
#define bcm2836_mmio_read(reg) mmio_read(BCM2836_MMIO_BASE + (reg))

// ARM data memory barrier
#define dmb() asm volatile("mcr p15, #0, %0, c7, c10, #5" :: "r" (0))

// Memory-Mapped I/O output
static inline void mmio_write(uint32_t reg, uint32_t data) {
	*(volatile uint32_t*)reg = data;
	dmb();
}

// Memory-Mapped I/O input
static inline uint32_t mmio_read(uint32_t reg) {
	dmb();
	return *(volatile uint32_t*)reg;
}
#endif
