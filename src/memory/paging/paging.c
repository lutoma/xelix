/* paging.c: Enables and controls paging
 * Copyright © 2010 Christoph Sünderhauf
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

#include "paging.h"

#include <common/log.h>
#include "frames.h"
#include "fault.h"
#include <memory/kmalloc.h>

static void switchPageDirectory(pageDirectory_t* directory)
{
	currentDirectory = directory;
	asm volatile("mov %0, %%cr3":: "r"(currentDirectory->physicalAddress));
}

static void allocatePage(pageTableEntry_t* page)
{
	page->frame = frames_allocateFrame();
	page->present = 1;
}

static void createPage(pageDirectory_t* directory, uint32 virtualAddress, enum mode usermode, enum readandwrite rw)
{
	uint32 totalPageNum = virtualAddress / 0x1000; // rounds down
	uint32 pageTableNum = totalPageNum / 1024;
	uint32 pageNumInTable = totalPageNum % 1024;
	
	
	if(directory->pageTables[pageTableNum] == 0)
	{
		// create page table!
		
		uint32 physAddressOfPageTable;
		pageTable_t* pageTable = kmalloc_aligned(sizeof(pageTable_t), &physAddressOfPageTable);
		// clear page table
		// this automatically sets all present-bits to zero, so this new page table will really work right away
		memset(pageTable, 0, sizeof(pageTable_t));
		
		directory->pageTables[pageTableNum] = pageTable;
		directory->directoryEntries[pageTableNum].pagetable = physAddressOfPageTable  / 0x1000;
		directory->directoryEntries[pageTableNum].present = 1;
		directory->directoryEntries[pageTableNum].rw = READWRITE;
		directory->directoryEntries[pageTableNum].usermode = USER_MODE;
	}
	
	pageTableEntry_t* page = &(directory->pageTables[pageTableNum]->tableEntries[pageNumInTable]);
	
	page->usermode = usermode;
	page->rw = rw;
	
	allocatePage(page);
}

pageDirectory_t* paging_cloneCurrentDirectory()
{
	// create directory (similiear to paging_init())
	uint32 tmpPhys;
	pageDirectory_t* directory = kmalloc_aligned(sizeof(pageDirectory_t), &tmpPhys);
	memset(directory, 0, sizeof(pageDirectory_t));
	directory->physicalAddress = tmpPhys + ((uint32)&(directory->directoryEntries) - (uint32)directory);
	
	// iterate through pagetables
	int tableNum;
	for(tableNum = 0; tableNum < 1024; tableNum++)
	{
		if(currentDirectory->pageTables[tableNum] == 0)
		{
			// page table not present in source
			continue;
		}
		if(currentDirectory->pageTables[tableNum] == kernelDirectory->pageTables[tableNum])
		{
			log("paging: link page table %d\n",tableNum);
			// link page table
			directory->pageTables[tableNum] = currentDirectory->pageTables[tableNum];
			directory->directoryEntries[tableNum] = currentDirectory->directoryEntries[tableNum];
		}
		else
		{
			
			log("paging: copy page table %d\n", tableNum);
			pageTable_t* srcTable = currentDirectory->pageTables[tableNum];
			// copy page table contents
			int pageNum;
			for(pageNum=0; pageNum < 1024; pageNum++)
			{
				if(srcTable->tableEntries[pageNum].present == 0) // when we implement swapping we need to change this
				{
					// page is empty
					continue;
				}
				createPage(directory, 0x1000*1024*tableNum + 0x1000*pageNum, KERNEL_MODE, READWRITE);
				// actually copy data
				
				// disable paging    this code will still work as we identity-mapped the kernel space
				uint32 cr0;
				asm volatile("mov %%cr0, %0": "=r"(cr0));
				cr0 &= ~0x80000000; // Enable paging!
				asm volatile("mov %0, %%cr0":: "r"(cr0));
				
				
				memcpy((uint32*) ( directory->pageTables[tableNum]->tableEntries[pageNum].frame * 0x1000), (uint32*) (currentDirectory->pageTables[tableNum]->tableEntries[pageNum].frame * 0x1000), 0x1000);  //->pageTables[tableNum] points to the current table even if paging is disabled, because we store page tables in the kernel memory which is identity mapped
				
				// enable paging again
				asm volatile("mov %%cr0, %0": "=r"(cr0));
				cr0 |= 0x80000000; // Enable paging!
				asm volatile("mov %0, %%cr0":: "r"(cr0));
			}
			
		}
	}
	return directory;
}

static void enable()
{
	uint32 cr0;
	asm volatile("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000;
	asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void paging_init()
{
	// create kernelDirectory
	uint32 tmpPhys;
	kernelDirectory = kmalloc_aligned(sizeof(pageDirectory_t), &tmpPhys);
	memset(kernelDirectory, 0, sizeof(pageDirectory_t));
	kernelDirectory->physicalAddress = tmpPhys + ((uint32)&(kernelDirectory->directoryEntries) - (uint32)kernelDirectory); // we need to put the physical location of kernelDirectory->directoryEntries into kernelDirectory->physicalAddress.
	
	// allocating and stuff works on the currentDirectory
	currentDirectory = kernelDirectory;
	
	// identity mapping of kernel memory space:
	// create pages for every virtual address in our kernel memory space
	// because frames are allocated linearly, this will result in identity mapping.
	int i;
	for(i=0; i <= MEMORY_MAX_KMEM; i += 0x1000)
	{
		createPage(kernelDirectory, i, KERNEL_MODE, READWRITE);
	}
	
	switchPageDirectory(kernelDirectory);
	enable();
	paging_registerFaultHandler();
}
