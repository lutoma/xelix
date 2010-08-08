#include <memory/paging/paging.h>
#include <memory/paging/frames.h>
#include <memory/kmalloc.h>

// STRUCTURES

enum mode {
	USER_MODE = 1,
	KERNEL_MODE = 0
};

enum readandwrite {
	READONLY = 0,
	READWRITE = 1
};
	

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

// VARIABLES

// The page directory the kernel uses before it starts other processes with their own virtual address space and their own page directories. Containts all the pages the kernel will ever need - that is already containts present pages for the whole kernel heap / kmalloc-space / whatever.
pageDirectory_t* kernelDirectory=0;

// The current page directory. The pages which the kernel also uses (which are present in kernelDirectory) are linked in.
pageDirectory_t* currentPageDirectory=0;


// FUNCTION DECLERATIONS

// allocates physical memory for the given page
void allocatePage(pageTableEntry_t* page);

// creates the pageDirectoryEntry and allocates memory (via allocatePage()) for the page containing the virtualAddress
// if the corresponding pageTable does not exist yet, creates it and adds its pageDirectoryEntry to the currentPageDirectory.
void createPage(uint32 virtualAddress, enum mode usermode, enum readandwrite rw);

// FUNCTION DEFINITIONS

void paging_init()
{
	// create kernelDirectory
	uint32 tmpPhys;
	kernelDirectory = kmalloc_aligned(sizeof(pageDirectory_t), &tmpPhys);
	memset(kernelDirectory, 0, sizeof(pageDirectory_t));
	kernelDirectory->physicalAddress = tmpPhys + ((uint32)&(kernelDirectory->directoryEntries) - (uint32)kernelDirectory); // we need to put the physical location of kernelDirectory->directoryEntries into kernelDirectory->physicalAddress.
	
	// allocating and stuff works on the currentPageDirectory
	currentPageDirectory = kernelDirectory;
	
	
	// identity mapping of kernel memory space:
	// create pages for every virtual address in our kernel memory space
	// because frames are allocated linearly, this will result in identity mapping.
	int i;
	for(i=0; i <=  kernelMaxMemory; i += 0x1000)
	{
		createPage(i, KERNEL_MODE, READWRITE);
	}
	
	// TODO: PAGEFAULT-INTERRUPT
	
	
	// set paging directory
	asm volatile("mov %0, %%cr3":: "r"(kernelDirectory->physicalAddress));
	// enable paging!
	uint32 cr0;
	asm volatile("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000; // Enable paging!
	asm volatile("mov %0, %%cr0":: "r"(cr0));
}


void allocatePage(pageTableEntry_t* page)
{
	if( page->present )
	{
		//log("Trying to allocatePage(pageTableEntry_t*) which is already present!\n");
	}
	page->frame = frames_allocateFrame();
	page->present = 1;
}

void createPage(uint32 virtualAddress, enum mode usermode, enum readandwrite rw)
{
	uint32 totalPageNum = virtualAddress / 0x1000; // rundet ab
	uint32 pageTableNum = totalPageNum / 1024;
	uint32 pageNumInTable = totalPageNum % 1024;
	
	
	if(currentPageDirectory->pageTables[pageTableNum] == 0)
	{
		// create page table!
		
		uint32 physAddressOfPageTable;
		pageTable_t* pageTable = kmalloc_aligned(sizeof(pageTable_t), &physAddressOfPageTable);
		// clear page table
		// this automatically sets all present-bits to zero, so this new page table will really work right away
		memset(pageTable, 0, sizeof(pageTable_t));
		
		currentPageDirectory->pageTables[pageTableNum] = pageTable;
		currentPageDirectory->directoryEntries[pageTableNum].pagetable = physAddressOfPageTable  / 0x1000;
		currentPageDirectory->directoryEntries[pageTableNum].present = 1;
		currentPageDirectory->directoryEntries[pageTableNum].rw = READWRITE;
		currentPageDirectory->directoryEntries[pageTableNum].usermode = USER_MODE;
	}
	
	pageTableEntry_t* page = &(currentPageDirectory->pageTables[pageTableNum]->tableEntries[pageNumInTable]);
	
	page->usermode = usermode;
	page->rw = rw;
	
	allocatePage(page);
	
	
	
}

