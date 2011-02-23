#pragma once

/* Copyright © 2010 Christoph Sünderhauf
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

/************
 * TYPES
 ************/

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
} pageTableEntry_t; // describes a page

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

/*****************
 * FUNCTIONS
 *****************/
 
 
void paging_init();

// creates a new pageDirectory (which is not set active), copying the currentDirectory and linking the pages present in kernelDirectory
pageDirectory_t* paging_cloneCurrentDirectory();

// switches paging to use the specified directory. Does not en- or deable paging!
void paging_switchPageDirectory(pageDirectory_t* directory);
