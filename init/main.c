// Initialization code of kernel


#include <common/multiboot.h>
#include <common/generic.h>
#include <common/string.h>
#include <devices/display/interface.h>
#include <devices/serial/interface.h>
#include <devices/cpu/interface.h>
#include <devices/keyboard/interface.h>
#include <memory/interface.h>
#include <interrupts/interface.h>
#include <devices/pit/interface.h>
#include <memory/kmalloc.h>
#include <filesystems/interface.h>
#include <filesystems/memfs/interface.h>
#include <devices/pit/interface.h>
#include <processes/process.h>
#include <init/debugconsole.h>


void checkIntLenghts();
void readInitrd(uint32 initrd_location);
void calculateFibonacci();
void compilerInfo();

/** Read the initrd file supplied by the bootloader (usally GNU GRUB).
 * @param initrd_location The position of the initrd in the kernel
 */
void readInitrd(uint32 initrd_location)
{
}

/// Prints out compiler information, especially for GNU GCC
void compilerInfo()
{
	log("This release of Xelix was compiled %s %s", __DATE__, __TIME__);

	/* Test for GCC > 3.2.0 */
	#if GCC_VERSION > 30200
		log(" using GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
	#else
		log(" using an unknown compiler\n");
	#endif
}

// The main kernel function.
// (This is the first function called ever)
void kmain(multibootHeader_t *mboot_ptr)
{

	
	// descriptor tables have to be created first
	memory_init_preprotected(); // gdt
	interrupts_init(); // idt
	
	
	
	// check that our initrd was loaded by the bootloader and determine the addresses.
	ASSERT(mboot_ptr->mods_count > 0);
	uint32 initrd_location = *((uint32*)mboot_ptr->mods_addr);
	uint32 initrd_end = *(uint32*)(mboot_ptr->mods_addr+4);
	// Don't trample our module with placement accesses, please
	kmalloc_init(initrd_end);
	
	
	display_init();
	serial_init();
	log_init();

	display_setColor(0x0f);
	print("\n");
	print("                                   Xelix\n");
	print("\n");
	display_setColor(0x07);
	
	compilerInfo();	
	cpu_init();
	memory_init_postprotected();
	pit_init(50); //50Hz
	keyboard_init();
	fs_init();
	
	log("Reading Initrd...\n");
	readInitrd(initrd_location);
	print("finished listing files\n");

	display_setColor(0x0f);
	log("Xelix is up.\n");
	display_setColor(0x07);	

	printf("This %%should %s%% colored. The color code is %%%d%%.\n", 0x02, "be", 0x04, 0x02);
	
	//createProcess("debugconsole", &debugconsole_init);
	debugconsole_init();
	while(1){}
}

// Check if ints have the right length
void checkIntLenghts()
{
	log("Checking length of uint8... ");
	ASSERT(sizeof(uint8) == 1);
	log("Right\n");
	
	log("Checking length of uint16... ");
	ASSERT(sizeof(uint16) == 2);
	log("Right\n");
	
	log("Checking length of uint32... ");
	ASSERT(sizeof(uint32) == 4);
	log("Right\n");
}
