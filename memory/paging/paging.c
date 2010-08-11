#include <memory/paging/paging.h>
#include <memory/paging/frames.h>
#include <memory/kmalloc.h>
#include <interrupts/interface.h>

/////////////////////////////
// ENUMS

enum mode {
	USER_MODE = 1,
	KERNEL_MODE = 0
};

enum readandwrite {
	READONLY = 0,
	READWRITE = 1
};

/////////////////////////////
// VARIABLES

// The page directory the kernel uses before it starts other processes with their own virtual address space and their own page directories. Containts all the pages the kernel will ever need - that is already containts present pages for the whole kernel heap / kmalloc-space / whatever.
pageDirectory_t* kernelDirectory=0;

// The current page directory. The pages which the kernel also uses (which are present in kernelDirectory) are linked in.
pageDirectory_t* currentPageDirectory=0;


/////////////////////////////
// FUNCTION DECLERATIONS

// allocates physical memory for the given page
void allocatePage(pageTableEntry_t* page);

// creates the pageDirectoryEntry and allocates memory (via allocatePage()) for the page containing the virtualAddress
// if the corresponding pageTable does not exist yet, creates it and adds its pageDirectoryEntry to the currentPageDirectory.
void createPage(uint32 virtualAddress, enum mode usermode, enum readandwrite rw);

// handles a page fault interrupt
void pageFaultHandler(registers_t regs);

/////////////////////////////
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
	
	// register pagefault-Interrupt
	interrupt_registerHandler(0xE /*=14*/, &pageFaultHandler);
	
	
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


void pageFaultHandler(registers_t regs)
{
	// get the address for which the page fault occured
	uint32 faultingAddress;
	asm volatile("mov %%cr2, %0" : "=r" (faultingAddress));
	
	// read infomation from the error code
	
	uint8 notPresent = ! (regs.err_code & 0x1); // pagefault because page was not present?
	uint8 write = regs.err_code & 0x2; // pagefault caused by write (if unset->read)
	uint8 usermode = regs.err_code & 0x4; // during usermode (if unset->kernelmode)
	uint8 reservedoverwritten = regs.err_code & 0x8; // reserved bits overwritten (if set -> reserved bits were overwritten causing this page fault
	uint8 instructionfetch = regs.err_code & 0x10; // pagefault during instruction set (if set -> during instruction fetch)
	
	print("pagefault at ");
	printHex(faultingAddress);
	print(": ");
	if(notPresent) print("not present, ");
	if(write) print("write, ");
	if(!write) print("read, ");
	if(usermode) print("user-mode, ");
	if(!usermode) print("kernel-mode, ");
	if(reservedoverwritten) print("reserved bits overwritten, ");
	if(instructionfetch) print("during instruction fetch");
	
	if(notPresent)
	{
		print("->createPage()\n");
		// we can handle this pagefault by creating a new page
		// at the moment, everything is in kernel mode
		createPage(faultingAddress, KERNEL_MODE, READWRITE);
		
		// Flush the TLB (translation lookaside buffer) selectively for the new page created
		asm ("invlpg %0" : : "m" (faultingAddress) );
	}
	else
	{
		print("NOT HANDLING PAGEFAULT!\n");
	}
}


