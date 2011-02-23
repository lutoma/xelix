/* multiboot.c: Multiboot-related functions
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "multiboot.h"

#include <common/log.h>

void multiboot_printInfo(multibootHeader_t *pointer)
{
/*
	log("\n%%Multiboot information:%%\n", 0x0f);
	log("\tflags: %d\n", pointer->flags);
	log("\tmemLower: 0x%x\n", pointer->memLower);
	log("\tmemUpper: 0x%x\n", pointer->memUpper);
	//log("\tbootDevice: %s\n", pointer->bootDevice);
	//if(pointer->bootLoaderName != 0) //QEMU doesn't append a \0 to the cmdline, resulting in epic failure in printf().
		//log("\tcmdLine: %s\n", pointer->cmdLine);
	log("\tmodsCount: %d\n", pointer->modsCount);
	log("\tmodsAddr: 0x%x\n", pointer->modsAddr);
	log("\tnum: %d\n", pointer->num);
	log("\tsize: %d\n", pointer->size);
	log("\taddr: 0x%x\n", pointer->addr);
	log("\tshndx: %d\n", pointer->shndx);
	log("\tmmapLength: %d\n", pointer->mmapLength);
	log("\tmmapAddr: 0x%x\n", pointer->mmapAddr);
	log("\tdrivesLength: %d\n", pointer->drivesLength);
	log("\tdrivesAddr: 0x%x\n", pointer->drivesAddr);
	log("\tconfigTable: %d\n", pointer->configTable);
	if(pointer->bootLoaderName != 0)
		log("\tbootLoaderName: %s\n", pointer->bootLoaderName);
	else
		log("\tbootLoaderName: 0\n");
		log("\tapmTable: %d\n", pointer->apmTable);
	log("\tvbeControlInfo: %d\n", pointer->vbeControlInfo);
	log("\tvbeModeInfo: %d\n", pointer->vbeModeInfo);
	log("\tvbeMode: %d\n", pointer->vbeMode);
	log("\tvbeInterfaceSeg: %d\n", pointer->vbeInterfaceSeg);
	log("\tvbeInterfaceOff: %d\n", pointer->vbeInterfaceOff);
	log("\tvbeInterfaceLen: %d\n", pointer->vbeInterfaceLen);
	*/
}
