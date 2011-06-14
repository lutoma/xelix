/* acpi.c: ACPI power management
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
 * 
 * This Code is largely based on kaworu's post in the osdev.org board at
 * http://forum.osdev.org/viewtopic.php?t=16990.
 */

#include "acpi.h"
#include <lib/generic.h>
#include <lib/log.h>
#include <lib/datetime.h>

// Just a small macro to make things shorter
#define acpiError(args...) do { log("acpi:" args); return -1; } while(false); 

uint32_t*	smiCmd;
uint8_t		enableStr;
uint8_t		acpiDisableStr;
uint32_t*	pm1aCnt;
uint32_t*	pm1bCnt;
uint32_t	slpTypeA;
uint32_t	slpTypeB;
uint32_t	slpEn;
uint32_t	sciEn;
uint8_t		pm1CountLength;

struct RSDPtr
{
	uint8_t		signature[8];
	uint8_t		checksum;
	uint8_t		oemID[6];
	uint8_t		revision;
	uint32_t	*rsdtAddress;
} __attribute__((packed));

struct FACP
{
	uint8_t		signature[4];
	uint32_t	Length;
	uint8_t		reserved[32];
	uint32_t*	dsdt;
	uint8_t		reserved1[4];
	uint32_t*	smiCmd;
	uint8_t		enableStr;
	uint8_t		acpiDisableStr;
	uint8_t		reserved2[10];
	uint32_t*	pm1aCntBlk;
	uint32_t*	pm1bCntBlk;
	uint8_t		reserved3[17];
	uint8_t		pm1CountLength;
} __attribute__((packed));

// Check if the given address contains a valid RSDP.
static uint32_t* checkRSDP(uint32_t* ptr)
{
	char *sig = "RSD PTR ";
	struct RSDPtr *rsdp = (struct RSDPtr*)ptr;
	uint8_t* bptr;
	uint8_t check = 0;
	int i;

	if (memcmp(sig, rsdp, 8) == 0)
	{
		// Check RSDP checksum
		bptr = (uint8_t*)ptr;
		
		for (i = 0; i < sizeof(struct RSDPtr); i++)
		{
			check += *bptr;
			bptr++;
		}

		// Found valid RSDP.
		if (check == 0) {
			 if (rsdp->revision == 0)
				log("Found valid RSDP (ACPI 1).\n");
			else
				log("Found valid RSDP (ACPI 2).\n");

			return (uint32_t*)rsdp->rsdtAddress;
		}
	}

	return NULL;
}

// Finds the ACPI header and returns a pointer to the RSDT.
static uint32_t* getRSDT()
{
	uint32_t* addr;
	uint32_t* rsdp;

	// Scan the memory below 1mb for a RSDP signature
	for(addr = (uint32_t*)0x000E0000; (int32_t)addr < 0x00100000; addr += 0x10 / sizeof(addr))
	{
		rsdp = checkRSDP(addr);
		if (rsdp != NULL)
			return rsdp;
	}

	// The RM segment of the EBDA is at adress 0x40:0x0E.
	int ebda = *((short*)0x40E);
	
	// transform segment into linear address
	ebda = ebda * 0x10 & 0x000FFFFF;
 
	/* Search the Extended BIOS Data Area for the Root System
	 * Description Pointer signature.
	 */
	for(addr = (uint32_t*) ebda; (int32_t)addr < ebda+1024; addr += 0x10 / sizeof(addr))
	{
		rsdp = checkRSDP(addr);
		
		if(rsdp != NULL)
			return rsdp;
	}

	return NULL;
}

// Checks for a given header and validates checksum
static int checkHeader(uint32_t* ptr, char* header)
{
	if(memcmp(ptr, header, 4) == 0)
	{
		char* checkPtr = (char*)ptr;
		int len = *(ptr + 1);
		char check = 0;
		
		while(0 < len--)
		{
			check += *checkPtr;
			checkPtr++;
		}
		
		if(check == 0)
			return 0;
	}
	
	return -1;
}

static int enable()
{
	// Check if acpi is already enabled
	if((inw((uint32_t)pm1aCnt) & sciEn) != 0)
		return 0;

	// Check if acpi can be enabled
	if(smiCmd == 0 || enableStr == 0)
		acpiError("No known way to enable acpi.\n");
	
	// Send the ACPI enable command
	outb((uint32_t)smiCmd, enableStr);

	// Give 3 seconds time to enable acpi
	int i;
	for(i = 0; i < 4; i++ )
	{
		if((inw((uint32_t)pm1aCnt) & sciEn) == 1)
			break;

		sleep(1);
	}
			
	if(pm1bCnt != 0)
	{
		for(; i < 8; i++)
		{
			if((inw((uint32_t) pm1bCnt) & sciEn) == 1)
				break;

			sleep(1);
		}
	}
			
	if (i >= 8)
		acpiError("Couldn't enable.\n");

	return 0;
}

int acpi_init()
{
	uint32_t* ptr = getRSDT();

	// Check if ACPI is available on this pc (if the pointer is correct)
	if(ptr == NULL || checkHeader(ptr, "RSDT") == -1)
		acpiError("No ACPI available.\n");

	// The RSDT contains an unknown number of pointers to ACPI tables
	int entries = *(ptr + 1);
	entries = (entries - 36) / 4;
	
	// Skip header information
	ptr += 36 / 4;

	while(0 < entries--)
	{
		// Check if the desired table is reached
		if(checkHeader((uint32_t*)*ptr, "FACP") != 0)
			continue;
			
		entries = -2;
		struct FACP* facp = (struct FACP*)*ptr;

		if(checkHeader((uint32_t*)facp->dsdt, "DSDT") != 0)
			acpiError("Invalid DSDT.\n");
			
		// Search the \_S5 package in the dsdt
		char* S5Addr = (char*)facp->dsdt + 36; // skip header
		int dsdtLength = *(facp->dsdt+1) - 36;

		while(0 < dsdtLength--)
		{
			if(memcmp(S5Addr, "_S5_", 4) == 0)
				break;

			S5Addr++;
		}

		// Check if \_S5 was found
		if(dsdtLength <= 0)
			acpiError("\\_S5 not present.\n");
		
		// Check for valid AML structure
		if((*(S5Addr-1) != 0x08 && ( *(S5Addr-2) != 0x08 || *(S5Addr-1) == '\\') ) || *(S5Addr+4) != 0x12)
			acpiError("\\_S5 parse error.\n");		

		S5Addr += 5;
			
		// Calculate PkgLength size
		S5Addr += ((*S5Addr &0xC0) >> 6) + 2;

		// Skip byteprefix
		if (*S5Addr == 0x0A)
			S5Addr++;

		slpTypeA = *(S5Addr) << 10;
		S5Addr++;

		// Skip byteprefix
		if (*S5Addr == 0x0A)
			S5Addr++;

		slpTypeB = *(S5Addr)<<10;
		smiCmd = facp->smiCmd;

		enableStr = facp->enableStr;
		acpiDisableStr = facp->acpiDisableStr;
		pm1aCnt = facp->pm1aCntBlk;
		pm1bCnt = facp->pm1bCntBlk;
		pm1CountLength = facp->pm1CountLength;

		slpEn = 1 << 13;
		sciEn = 1;

		return 0;

		ptr++;
	}

	acpiError("no valid FACP present.\n");
}

void acpi_powerOff()
{
	// sciEn is true if ACPI shutdown is possible.
	if (!sciEn)
		return;

	enable();

	// Send the shutdown command
	outw((unsigned int)pm1aCnt, slpTypeA | slpEn);

	if(pm1bCnt != 0)
		outw((unsigned int)pm1bCnt, slpTypeB | slpEn);

	log("acpi: PowerOff failed.\n");
}
