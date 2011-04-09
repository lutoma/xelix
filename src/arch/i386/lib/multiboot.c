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

#include <arch/i386/lib/multiboot.h>

#include <lib/log.h>
#include <lib/string.h>

static void printInfo(uint32_t var, char* varname, char type)
{
	// Lazy coder was lazy
	char* t;
	if(type == 's')
		t = "multiboot: %s: %s\n";
	else
		t = "multiboot: %s: %d\n";

	log(t, varname, var);
}

#define printMbootStr(C) printInfo(C, #C, 's');
#define printMbootInt(C) printInfo(C, #C, 'd');
#define printMbootHex(C) printInfo(C, #C, 'd');

void arch_multiboot_printInfo()
{
	printMbootHex(multiboot_info->flags);
	printMbootInt(multiboot_info->memLower);
	printMbootInt(multiboot_info->memUpper);
	printMbootHex(multiboot_info->bootDevice);
	printMbootStr((uint32_t)multiboot_info->cmdLine);
	printMbootInt(multiboot_info->modsCount);
	printMbootHex((uint32_t)multiboot_info->modsAddr);


	printMbootInt(multiboot_info->mmapLength);
	printMbootHex(multiboot_info->mmapAddr);
	
	printMbootInt(multiboot_info->drivesLength);
	printMbootHex(multiboot_info->drivesAddr);
	
	// ROM configuration table
	printMbootHex(multiboot_info->configTable);
	
	printMbootStr((uint32_t)multiboot_info->bootLoaderName);
	printMbootHex(multiboot_info->apmTable);
	
	// Video
	printMbootInt(multiboot_info->vbeControlInfo);
	printMbootInt(multiboot_info->vbeModeInfo);
	printMbootInt(multiboot_info->vbeMode);
	printMbootHex(multiboot_info->vbeInterfaceSeg);
	printMbootHex(multiboot_info->vbeInterfaceOff);
	printMbootInt(multiboot_info->vbeInterfaceLen);
}
