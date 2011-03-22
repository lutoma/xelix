/* cpuid.c: Fetching CPUID information.
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

#include "cpuid.h"

#include <lib/log.h>
#include <memory/kmalloc.h>
#include <lib/string.h>

extern bool __cdecl cpuid_check(); // ASM.

cpuid_t* cpuid_data;

static void sendCommand(uint32 command)
{
	if(command == 0)
		// OR-ing eax with itself = setting it to 0. Faster than mov.
		asm("xor eax, eax");
	else
		asm("mov eax, %0" :: "r" (command) : "eax");

	asm("cpuid");
}

void cpuid_init()
{
	if(!cpuid_check())
	{
		log("cpuid: No CPUID support, exiting.\n");
		return;
	}
	
	cpuid_data = (cpuid_t*)kmalloc(sizeof(cpuid_t));
	memset(cpuid_data, 0, sizeof(cpuid_t));
	
	printf("cpuid: Reading Vendor id.\n");
	
	char tmp[5];
	tmp[4] = 0;
	
	sendCommand(0);
	asm("mov %0, ebx" : "=m" (tmp));
	strcat(cpuid_data->vendor, tmp);
	
	sendCommand(0);
	asm("mov %0, edx" : "=m" (tmp));
	strcat(cpuid_data->vendor, tmp);
	
	sendCommand(0);
	asm("mov %0, ecx" : "=m" (tmp));
	strcat(cpuid_data->vendor, tmp);
	
	cpuid_data->vendor[12] = 0;
	
	log("cpuid: vendor: %s\n", cpuid_data->vendor);
	
	if(!strcmp(cpuid_data->vendor, "GenuineIntel"))
		cpuid_data->isIntel = true;
	else if(!strcmp(cpuid_data->vendor, "AuthenticAMD"))
		cpuid_data->isAMD = true;
		
	
}
