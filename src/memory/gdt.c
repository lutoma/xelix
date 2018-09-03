/* gdt.c: Disable segmentation by defining one large segment
 * Copyright © 2010 Christoph Sünderhauf
 * Copyright © 2011-2018 Lukas Martini
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

#include "gdt.h"

struct entry {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;

	// Access flags, determine what ring this segment can be used in.
	uint8_t access;
	uint8_t granularity;

	uint8_t base_high;
} __attribute__((packed));

struct pointer {
	// The upper 16 bits of all selector limits.
	uint16_t limit;
	void* base;
} __attribute__((packed));

// gdt.asm
extern void gdt_flush(void*);

static struct entry entries[5];
static struct pointer pointer;

static inline void set_gate(int32_t num, uint32_t base, uint32_t limit,
	uint8_t access, uint8_t gran) {

	entries[num].base_low = (base & 0xffff);
	entries[num].base_mid = (base >> 16) & 0xff;
	entries[num].base_high = (base >> 24) & 0xff;

	entries[num].limit_low = (limit & 0xffff);
	entries[num].granularity = (limit >> 16) & 0x0f;

	entries[num].granularity |= gran & 0xf0;
	entries[num].access = access;
}

void gdt_init() {
	pointer.limit = (sizeof(struct entry) * 5) - 1;
	pointer.base = &entries;

	set_gate(0, 0, 0, 0, 0);
	set_gate(1, 0, 0xffffffff, 0x9a, 0xcf); // 0x04: User mode Code segment
	set_gate(2, 0, 0xffffffff, 0x92, 0xcf); // 0x08: Kernel Code segment
	set_gate(3, 0, 0xffffffff, 0xfa, 0xcf); // 0x0C: User mode Data segment
	set_gate(4, 0, 0xffffffff, 0xf2, 0xcf); // 0x10: Kernel Data segment

	gdt_flush(&pointer);
}
