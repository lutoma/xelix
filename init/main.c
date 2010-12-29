// Initialization code of kernel

#include <buildinfo.h>
#include <common/multiboot.h>
#include <common/generic.h>
#include <common/log.h>
#include <common/datetime.h>
#include <common/string.h>
#include <devices/display/interface.h>
#ifdef WITH_SERIAL
#include <devices/serial/interface.h>
#endif
#include <devices/cpu/interface.h>
#include <devices/keyboard/interface.h>
#include <memory/interface.h>
#include <interrupts/interface.h>
#include <devices/pit/interface.h>
#include <memory/kmalloc.h>
#ifdef WITH_SPEAKER
#include <devices/speaker/interface.h>
#endif
#include <filesystems/vfs.h>
#include <filesystems/memfs/interface.h>
#include <processes/process.h>

#ifdef WITH_DEBUGCONSOLE
#include <init/debugconsole.h>
#endif

// Prints out compiler information, especially for GNU GCC
static void compilerInfo()
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

// Check if ints have the right length
static void checkIntLenghts()
{
	log("init: Checking length of uint8... ");
	ASSERT(sizeof(uint8) == 1);
	log("Right\n");
	
	log("init: Checking length of uint16... ");
	ASSERT(sizeof(uint16) == 2);
	log("Right\n");
	
	log("init: Checking length of uint32... ");
	ASSERT(sizeof(uint32) == 4);
	log("Right\n");
}

#ifdef WITH_SPEAKER
static void bootBeep()
{
	speaker_beep(1, 1);
}
#endif

// The main kernel function.
// (This is the first function called ever)
void kmain(multibootHeader_t *mbootPointer)
{

	// descriptor tables have to be created first
	memory_init_preprotected(); // gdt
	interrupts_init(); // idt	

/*
 * This should be
 * kmalloc_init(mbootPointer->modsAddr);
 * however, mbootPointer->modsAddr always was 0, therefore i replaced it by this dirty hack.
 * Fix ASAP!
 */
	kmalloc_init(500);
	display_init();
	
	IFDEFC(WITH_SERIAL, serial_init());
	log_init();

	printf("\n                                   %%Xelix%%\n\n", 0x0f);

	compilerInfo();
	checkIntLenghts();
	multiboot_printInfo(mbootPointer);

	pit_init(PIT_RATE);
	cpu_init();
	memory_init_postprotected();
	keyboard_init();
	IFDEFC(WITH_SPEAKER, speaker_init());
	fs_init();

	//IFDEFC(WITH_SPEAKER, createProcess("bootBeep", &bootBeep));

	log("%%Xelix is up.%%\n", 0x0f);

	IFDEFC(WITH_DEBUGCONSOLE, createProcess("debugconsole", &debugconsole_init));
	
	while(1){}
}
