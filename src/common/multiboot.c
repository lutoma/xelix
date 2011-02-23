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

static void printInfo(var, varname, type)
{
	// Chop off the pointer->
	varname = substr(varname, 9, strlen(varname)-9);
	
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

void multiboot_printInfo(multibootHeader_t *pointer)
{

	log("\n%%Multiboot information:%%\n", 0x0f);
	printMbootInt(pointer->flags);
	printMbootInt(pointer->memLower);
	printMbootInt(pointer->memUpper);
	printMbootStr(pointer->bootDevice);
	printMbootStr(pointer->cmdLine);
	printMbootInt(pointer->modsCount);
	printMbootInt(pointer->modsAddr);
	printMbootInt(pointer->num);
	printMbootInt(pointer->size);
	printMbootInt(pointer->addr);
	printMbootInt(pointer->shndx);
	printMbootInt(pointer->mmapLength);
	printMbootInt(pointer->mmapAddr);
	printMbootInt(pointer->drivesLength);
	printMbootInt(pointer->drivesAddr);
	printMbootInt(pointer->configTable);
	printMbootStr(pointer->bootLoaderName);
	printMbootInt(pointer->apmTable);
	printMbootInt(pointer->vbeControlInfo);
	printMbootInt(pointer->vbeModeInfo);
	printMbootInt(pointer->vbeMode);
	printMbootInt(pointer->vbeInterfaceSeg);
	printMbootInt(pointer->vbeInterfaceOff);
	printMbootInt(pointer->vbeInterfaceLen);
}
