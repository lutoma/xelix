/* portio.c: CPU port io.
 * Copyright © 2011 Lukas Martini
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

#include "portio.h"
#include <lib/stdint.h>

#define outMacro(name, type) \
	void name (uint16_t port, type value) \
	{ \
		asm("out %0, %1" : : "Nd" (port), "a" (value)); \
	}


#define inMacro(name, type) \
	type name (uint16_t port) \
	{ \
		type ret; \
		asm ("in %0, %1" : "=a" (ret) : "Nd" (port)); \
		return ret; \
	}

outMacro(portio_out8, uint8_t)
outMacro(portio_out16, uint16_t)
outMacro(portio_out32, uint32_t)
outMacro(portio_out64, uint64_t)

inMacro(portio_in8, uint8_t)
inMacro(portio_in16, uint16_t)
inMacro(portio_in32, uint32_t)
inMacro(portio_in64, uint64_t)
