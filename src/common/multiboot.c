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
#include <common/string.h>

static void printInfo(uint32 var, char* varname, char type)
{
	// Chop off the multiboot_header->
	varname = substr(varname, 18, strlen(varname) -18);
	
	// Lazy coder was lazy
	char* t;
	if(type == 's')
		t = "\t %s: %s\n";
	else
		t = "\t %s: %d\n";

	if(var == NULL)
		printf(t, varname, var);
	else
		printf(t, varname, var);
}

#define printMbootStr(C) printInfo(C, #C, 's');
#define printMbootInt(C) printInfo(C, #C, 'd');

void multiboot_printInfo()
{
	log("\n%%Multiboot information:%%\n", 0x0f);
	printMbootInt(multiboot_header->flags);
	printMbootInt(multiboot_header->memLower);
	printMbootInt(multiboot_header->memUpper);
	printMbootStr(multiboot_header->bootDevice);
	printMbootStr(multiboot_header->cmdLine);
	printMbootInt(multiboot_header->modsCount);
	printMbootInt(multiboot_header->modsAddr);
	printMbootInt(multiboot_header->num);
	printMbootInt(multiboot_header->size);
	printMbootInt(multiboot_header->addr);
	printMbootInt(multiboot_header->shndx);
	printMbootInt(multiboot_header->mmapLength);
	printMbootInt(multiboot_header->mmapAddr);
	printMbootInt(multiboot_header->drivesLength);
	printMbootInt(multiboot_header->drivesAddr);
	printMbootInt(multiboot_header->configTable);
	printMbootStr(multiboot_header->bootLoaderName);
	printMbootInt(multiboot_header->apmTable);
	printMbootInt(multiboot_header->vbeControlInfo);
	printMbootInt(multiboot_header->vbeModeInfo);
	printMbootInt(multiboot_header->vbeMode);
	printMbootInt(multiboot_header->vbeInterfaceSeg);
	printMbootInt(multiboot_header->vbeInterfaceOff);
	printMbootInt(multiboot_header->vbeInterfaceLen);
}

void multiboot_init(multibootHeader_t* pointer)
{
	multiboot_header = pointer;
}
