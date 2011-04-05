#pragma once

/* Copyright © 2011 Lukas Martini
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

#include <lib/generic.h>

#define CPUID_VENDOR_UNKNOWN	0
#define CPUID_VENDOR_AMD		1
#define CPUID_VENDOR_CENTAUR	2
#define CPUID_VENDOR_CYRIX		3
#define CPUID_VENDOR_INTEL		4
#define CPUID_VENDOR_NEXGEN		5
#define CPUID_VENDOR_NSC		6
#define CPUID_VENDOR_RISE		7
#define CPUID_VENDOR_SIS		8
#define CPUID_VENDOR_TRANSMETA	9
#define CPUID_VENDOR_UMC		10
#define CPUID_VENDOR_VIA		11

/* This is by far not complete and only contains what I've implemented
 * so far. Feel free to add things. -- Lukas
 */
typedef struct {
	char	vendorName[13];
	uint32_t	vendor;
	uint32_t	lastFunction;
	char*	cpuName;
} cpuid_t;

cpuid_t* cpuid_data;

void cpuid_init();
