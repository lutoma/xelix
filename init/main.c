/** @file init/main.c
 * \brief Initialization code of kernel
 * @author Lukas Martini
 * @author Christoph Sünderhauf
 */

#include <common/multiboot.h>
#include <common/generic.h>
#include <common/string.h>
#include <devices/display/interface.h>
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
	log("This release of Xelix was compiled ");
	log(__DATE__);
	print(" ");
	log(__TIME__);

	/* Test for GCC > 3.2.0 */
	#if GCC_VERSION > 30200
		log(" using GCC ");
		logDec(__GNUC__);
		log(".");
		logDec(__GNUC_MINOR__);
		log(".");
		logDec(__GNUC_PATCHLEVEL__);
		log("\n");
	#else
		log(" using an unknown compiler\n");
	#endif
}
/** The main kernel function.
 * This is called first.
 * @param mboot_ptr Pointer to the multiboot header
 */
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
	log_init();

	display_setColor(0x0f);
	print("\n");
	print("                                   Xelix\n");
	print("\n");
	display_setColor(0x07);
	
	compilerInfo();	
	log("Initialized preprotected memory\n"); // cheating. this already happened, but display wasn't up yet.
	log("Initialized interrupts\n"); // same here
	log("Initialized Display.\n");  // same here, surprise...
	cpu_init();
	log("Initialized CPU\n");
	memory_init_postprotected();
	log("Initialized postprotected memory\n");
	pit_init(50); //50Hz
	log("Initialized PIT (programmable interrupt timer)\n");
	keyboard_init();
	log("Initialized keyboard\n");
	
	log("Reading Initrd...\n");
	readInitrd(initrd_location);

	print("finished listing files\n");

	display_setColor(0x0f);
	log("Xelix is up.\n");
	display_setColor(0x07);	

	createProcess("debugconsole", &debugconsole_init);
	
	while(1)
	{
		//print("main kernel loop\n");
	}
}

/// Check if ints have the right length
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
