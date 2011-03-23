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

#define CMD_VENDOR			0
#define CMD_GET_LAST_CMD	0x80000000
#define CMD_INTEL_CPUNAME	0x80000002

extern bool __cdecl cpuid_check(); // ASM.

#define VENDORNUM 11

char vendors[VENDORNUM][13] =
{
	"AuthenticAMD",
	"CentaurHauls",
	"CyrixInstead",
	"GenuineIntel",
	"NexGenDriven",
	"Geode by NSC",
	"RiseRiseRise",
	"SiS SiS SiS ",
	"GenuineTMx86",
	"UMC UMC UMC ",
	"VIA VIA VIA "
};

/* Sends command and calls cpuid command. Also checks for maximum
 * supported function number.
 */
static void sendCommand(uint32 command)
{
	// If command > highest supported function and already initialized.
	if(cpuid_data->lastFunction \
		&& command != CMD_GET_LAST_CMD \
		&& command > cpuid_data->lastFunction)
		return;
	
	if(command == 0)
		// OR-ing eax with itself = setting it to 0. Faster than mov.
		asm("xor eax, eax");
	else
		asm("mov eax, %0" :: "r" (command) : "eax");

	asm("cpuid");
}

/* Converts vendor id we get from the CPU to a vendor num which we use
 * internally.
 */
static uint32 vendorNameToVendor(char vendorName[13])
{
	uint32 i;
	for(i = 0; i < VENDORNUM; i++)
		if(!strcmp(vendorName, vendors[i]))
			return i + 1;
	
	return 0;
}

// TODO: Find out why we have to re-send the command over and over again.
void cpuid_init()
{
	cpuid_data = (cpuid_t*)kmalloc(sizeof(cpuid_t));
	memset(cpuid_data, 0, sizeof(cpuid_t));
	
	if(!cpuid_check())
	{
		log("cpuid: No CPUID support, exiting.\n");
		return;
	}

	/* Get the highest supported _normal_ function number. There are
	 * extended ones for which we'll check later (However, for intel
	 * both seems to be the same).
	 */
	sendCommand(CMD_VENDOR);
	asm("mov %0, eax" : "=m" (cpuid_data->lastFunction));
	log("cpuid: Highest supported function number: 0x%x.\n", cpuid_data->lastFunction);
	
	char tmp[5] = "    "; // sic.
	
	sendCommand(CMD_VENDOR);
	asm("mov %0, ebx" : "=m" (tmp));
	strcat(cpuid_data->vendorName, tmp);
	
	sendCommand(CMD_VENDOR);
	asm("mov %0, edx" : "=m" (tmp));
	strcat(cpuid_data->vendorName, tmp);
	
	sendCommand(CMD_VENDOR);
	asm("mov %0, ecx" : "=m" (tmp));
	strcat(cpuid_data->vendorName, tmp);
	
	cpuid_data->vendorName[12] = 0;
	
	log("cpuid: vendorName: %s\n", cpuid_data->vendorName);
	
	cpuid_data->vendor = vendorNameToVendor(cpuid_data->vendorName);
	log("cpuid: vendor: %d.\n", cpuid_data->vendor);

	if(cpuid_data->vendor == CPUID_VENDOR_INTEL || cpuid_data->vendor == CPUID_VENDOR_AMD)
	{
		sendCommand(CMD_GET_LAST_CMD);
		asm("mov %0, eax" : "=m" (cpuid_data->lastFunction));
		log("cpuid: Highest supported extended function number: 0x%x.\n", cpuid_data->lastFunction);
	}

}
