/* main.c: Initialization code of the kernel
 * Copyright © 2010, 2011 Lukas Martini
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

#include <buildinfo.h>
#include <lib/multiboot.h>
#include <lib/generic.h>
#include "init.h"
#include <lib/log.h>
#include <lib/datetime.h>
#include <lib/string.h>
#include <hw/display.h>
#ifdef WITH_SERIAL
#include <hw/serial.h>
#endif
#include <hw/cpu.h>
#include <hw/keyboard.h>
#include <memory/interface.h>
#include <interrupts/interface.h>
#include <hw/pit.h>
#include <memory/kmalloc.h>
#ifdef WITH_SPEAKER
#include <hw/speaker.h>
#endif
#include <filesystems/vfs.h>
#include <filesystems/memfs/interface.h>
#include <tasks/task.h>
#include <lib/argparser.h>

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

void kmain(multibootHeader_t *mbootPointer)
{
	memory_init_preprotected();
	init(interrupts);

	init(kmalloc);
	init(display);
	if(WITH_SERIAL) init(serial);
	init(log);
	
	compilerInfo();
	checkIntLenghts();
	init(multiboot, mbootPointer);
	multiboot_printInfo(mbootPointer);

	init(argparser, (char*)mbootPointer->cmdLine);

	if(mbootPointer->bootLoaderName != NULL && find_substr((char*)mbootPointer->bootLoaderName, "GNU GRUB") != -1)
		init_haveGrub = true;
	else
		log("init: It looks like you don't use GNU GRUB as bootloader. Please note that we only support GRUB and things might be broken.\n");
	
	init(pit, PIT_RATE);
	init(cpu);
	memory_init_postprotected();

	if(mbootPointer->modsCount > 0)
	{
		init(vfs, (char**)mbootPointer->modsAddr);
	}	else
		PANIC("Could not load initrd (mbootPointer->modsCount <= 0)");

	init(keyboard);
	if(WITH_DEBUGCONSOLE) init(debugconsole);

	asm("sti");
	/* Just in case they're disabled for whatever reason.
	 * Note, just in case that's not obvious:
	 * If you disable interrupts in an interrupt handler and
	 * forget to re-enable them, we're basically screwed, so
	 * be a brave man and don't do that.
	 */
	while(true)
		asm("hlt"); // Wait until interrupt fires
}
