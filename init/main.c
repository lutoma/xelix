// Initialization code of kernel

#include <buildinfo.h>
#include <common/multiboot.h>
#include <common/generic.h>
#include <common/log.h>
#include <common/string.h>
#include <devices/display/interface.h>
#include <devices/serial/interface.h>
#include <devices/cpu/interface.h>
#include <devices/keyboard/interface.h>
#include <memory/interface.h>
#include <interrupts/interface.h>
#include <devices/pit/interface.h>
#include <memory/kmalloc.h>
#include <filesystems/vfs.h>
#include <filesystems/memfs/interface.h>
#include <devices/pit/interface.h>
#include <processes/process.h>
#include <init/debugconsole.h>


void checkIntLenghts();
void readInitrd(uint32 initrd_location);
void calculateFibonacci();
void compilerInfo();


/// Prints out compiler information, especially for GNU GCC
void compilerInfo()
{
	log("%%Compiling information:\n%%", 0x0f);
	log("\tTime: %s %s\n", __DATE__, __TIME__);
	// Test for GCC > 3.2.0
	#if GCC_VERSION > 30200
		log("\tCompiler: GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
	#else
		log("\tCompiler: Unknown\n");
	#endif
	log("\tBy: %s\n", __BUILDCOMP__);
	log("\tOS: %s\n", __BUILDSYS__);
	log("\tDistribution: %s\n", __BUILDDIST__);
}


// The main kernel function.
// (This is the first function called ever)
void kmain(multibootHeader_t *mbootPointer)
{

	// descriptor tables have to be created first
	memory_init_preprotected(); // gdt
	interrupts_init(); // idt	

	kmalloc_init(mbootPointer->modsAddr);
	display_init();
	serial_init();
	log_init();

	printf("\n                                   %%Xelix%%\n\n", 0x0f);
	
	compilerInfo();	
	multiboot_printInfo(mbootPointer);
	cpu_init();
	memory_init_postprotected();
	pit_init(50); //50Hz
	keyboard_init();
	fs_init();

	log("%%Xelix is up.%%\n", 0x0f);

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
