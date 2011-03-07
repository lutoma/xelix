#pragma once

/* Copyright © 2010 Christoph Sünderhauf
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

#include <common/generic.h>

typedef struct {
	uint32 present  :1;   // Page present in memory 
	uint32 rw       :1;   // Read-only if clear, readwrite if set (only applies to code running in user-mode, kernel-mode can do everything ;) )
	uint32 usermode :1;   // Supervisor level only if clear
	uint32 w        :1;   // write through - set: write-though caching, unset:write back
	uint32 cachedis :1;   // cache disabled. set: page won't be cached
	uint32 accessed :1;   // Has the page been accessed since last refresh?
	uint32 dirty    :1;   // Has the page been written to since last refresh?
	uint32 zero     :1;   // always zero
	uint32 global   :1;   // something complicated - leave it disabled ;)
	uint32 available:3;   // free for our use
	uint32 frame    :20;  // physical frame address (shifted right 12 bits->aligned to 4kb), ie. frame number
} pageTableEntry_t;

typedef struct {
	pageTableEntry_t tableEntries[1024];
} pageTable_t;

typedef struct {
	uint32 present  :1;   // Page present in memory 
	uint32 rw       :1;   // Read-only if clear, readwrite if set 
	uint32 usermode :1;   // Supervisor level only if clear
	uint32 w        :1;   // write through - set: write-though caching, unset:write back
	uint32 cachedis :1;   // cache disabled. set: page won't be cached
	uint32 accessed :1;   // Has the page been accessed since last refresh?
	uint32 zero     :1;   // always zero
	uint32 pagesize :1;   // 0 for 4kb
	uint32 ignored  :1;
	uint32 available:3;   // free for our use
	uint32 pagetable:20;  // physical address of page table (shifted right 12 bits->aligned to 4kb)
} pageDirectoryEntry_t; // points to a page table

typedef struct {
	pageDirectoryEntry_t directoryEntries[1024];
	
	// the cpu only needs a the physical address to above directoryEntries, so
	// the page directory on a hardware level just consists of the upper
	// directoryEntries.
	// but we need to store some additional meta-information:
	
	pageTable_t* pageTables[1024]; // direct pointers (i.e. virtual addresses) to the page tables, because the directoryEntries contain the physical address of the page tables and we need virtual addresses to access them in the kernel. If the present bit in the corresponding directoryEntries[i] is 0, then pageTables[i] be zero.
	
	uint32 physicalAddress; // the physical address of the directoryEntries[] array, because we need it to give it to the cpu.
} pageDirectory_t;

enum mode {
	USER_MODE = 1,
	KERNEL_MODE = 0
};

enum readandwrite {
	READONLY = 0,
	READWRITE = 1
};

// The page directory the kernel uses before it starts other processes with their own virtual address space and their own page directories. Containts all the pages the kernel will ever need - that is already containts present pages for the whole kernel heap / kmalloc-space / whatever.
pageDirectory_t* kernelDirectory;

// The current page directory. The pages which the kernel also uses (which are present in kernelDirectory) are linked in.
pageDirectory_t* currentDirectory;

pageDirectory_t* paging_cloneCurrentDirectory();
void paging_init();
