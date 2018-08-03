#pragma once

/* Copyright © 2010-2018 Lukas Martini
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

#include <lib/generic.h>

#define MULTIBOOT_KERNELMAGIC 0x2BADB002

#define MULTIBOOT_FLAG_MEM	 0x001
#define MULTIBOOT_FLAG_DEVICE  0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS	0x008
#define MULTIBOOT_FLAG_AOUT	0x010
#define MULTIBOOT_FLAG_ELF	 0x020
#define MULTIBOOT_FLAG_MMAP	0x040
#define MULTIBOOT_FLAG_CONFIG  0x080
#define MULTIBOOT_FLAG_LOADER  0x100
#define MULTIBOOT_FLAG_APM	 0x200
#define MULTIBOOT_FLAG_VBE	 0x400

struct multiboot_memory_map {
	uint32_t size;
	uint64_t addr;
	uint64_t length;
	uint32_t type;
} __attribute__((packed));

struct multiboot_module {
	uint32_t start;
	uint32_t end;
	char* cmdline;
	uint32_t reserved;
} __attribute__((packed)) ;

typedef struct {
	uint32_t flags;
	uint32_t mem_lower;
	uint32_t mem_upper;
	uint32_t boot_device;
	char* cmdline;
	uint32_t mods_count;
	struct multiboot_module* mods_addr;

	uint32_t elf_num;
	uint32_t elf_size;
	uint32_t elf_addr;
	uint32_t elf_shndx;

	uint32_t mmap_length;
	uint32_t mmap_addr;
	uint32_t drives_length;
	uint32_t drives_addr;

	uint32_t config_table;

	char* bootLoaderName;
	uint32_t apmTable;

	// Video
	uint32_t vbeControlInfo;
	uint32_t vbeModeInfo;
	uint16_t vbeMode;
	uint16_t vbeInterfaceSeg;
	uint16_t vbeInterfaceOff;
	uint16_t vbeInterfaceLen;
} __attribute__((packed)) multiboot_info_t;
