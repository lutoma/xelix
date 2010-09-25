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
pageDirectory_t* currentDirectory=0;


/////////////////////////////
// FUNCTION DECLERATIONS

// allocates physical memory for the given page
void allocatePage(pageTableEntry_t* page);

// creates the pageDirectoryEntry and allocates memory (via allocatePage()) for the page containing the virtualAddress in directory
// if the corresponding pageTable does not exist yet, creates it and adds its pageDirectoryEntry to the directory.
void createPage(pageDirectory_t* directory, uint32 virtualAddress, enum mode usermode, enum readandwrite rw);

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
	
	// allocating and stuff works on the currentDirectory
	currentDirectory = kernelDirectory;
	
	
	// identity mapping of kernel memory space:
	// create pages for every virtual address in our kernel memory space
	// because frames are allocated linearly, this will result in identity mapping.
	int i;
	for(i=0; i <=  kernelMaxMemory; i += 0x1000)
	{
		createPage(kernelDirectory, i, KERNEL_MODE, READWRITE);
	}
	
	// register pagefault-Interrupt
	interrupt_registerHandler(0xE /*=14*/, &pageFaultHandler);
	
	
	// set paging directory
	paging_switchPageDirectory(kernelDirectory);
	// enable paging!
	uint32 cr0;
	asm volatile("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000; // Enable paging!
	asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void paging_switchPageDirectory(pageDirectory_t* directory)
{
	currentDirectory = directory;
	asm volatile("mov %0, %%cr3":: "r"(currentDirectory->physicalAddress));
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

void createPage(pageDirectory_t* directory, uint32 virtualAddress, enum mode usermode, enum readandwrite rw)
{
	uint32 totalPageNum = virtualAddress / 0x1000; // rundet ab
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
		createPage(currentDirectory, faultingAddress, KERNEL_MODE, READWRITE);
		
		// Flush the TLB (translation lookaside buffer) selectively for the new page created
		asm ("invlpg %0" : : "m" (faultingAddress) );
	}
	else
	{
		print("NOT HANDLING PAGEFAULT!\n");
	}
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
			print("link page table ");
			printDec(tableNum);
			print("\n");
			// link page table
			directory->pageTables[tableNum] = currentDirectory->pageTables[tableNum];
			directory->directoryEntries[tableNum] = currentDirectory->directoryEntries[tableNum];
		}
		else
		{
			
			print("copy page table ");
			printDec(tableNum);
			print("\n");
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
