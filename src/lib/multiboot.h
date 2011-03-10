#pragma once

/* Copyright © 2010 Lukas Martini
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

#include "generic.h"

#define MULTIBOOT_FLAG_MEM     0x001
#define MULTIBOOT_FLAG_DEVICE  0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS    0x008
#define MULTIBOOT_FLAG_AOUT    0x010
#define MULTIBOOT_FLAG_ELF     0x020
#define MULTIBOOT_FLAG_MMAP    0x040
#define MULTIBOOT_FLAG_CONFIG  0x080
#define MULTIBOOT_FLAG_LOADER  0x100
#define MULTIBOOT_FLAG_APM     0x200
#define MULTIBOOT_FLAG_VBE     0x400

typedef struct {
	uint32 flags;
	uint32 memLower;
	uint32 memUpper;
	uint32 bootDevice;
	uint32 cmdLine;
	uint32 modsCount;
	uint32 modsAddr;
	uint32 num;
	uint32 size;
	uint32 addr;
	uint32 shndx;
	uint32 mmapLength;
	uint32 mmapAddr;
	uint32 drivesLength;
	uint32 drivesAddr;
	uint32 configTable;
	uint32 bootLoaderName;
	uint32 apmTable;
	uint32 vbeControlInfo;
	uint32 vbeModeInfo;
	uint32 vbeMode;
	uint32 vbeInterfaceSeg;
	uint32 vbeInterfaceOff;
	uint32 vbeInterfaceLen;
} __attribute__((packed))
multibootHeader_t;

multibootHeader_t* multiboot_header;

void multiboot_printInfo();
void multiboot_init(multibootHeader_t* pointer);
