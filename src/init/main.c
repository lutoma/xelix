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

#include "init.h"

#include <lib/multiboot.h>
#include <lib/log.h>
#include <lib/datetime.h>
#include <lib/string.h>
#include <hw/display.h>
#include <hw/serial.h>
#include <hw/cpu.h>
#include <hw/keyboard.h>
#include <memory/interface.h>
#include <interrupts/interface.h>
#include <hw/pit.h>
#include <memory/kmalloc.h>
#include <hw/speaker.h>
#include <filesystems/vfs.h>
#include <filesystems/memfs/interface.h>
#include <tasks/task.h>
#include <lib/argparser.h>
#include <init/debugconsole.h>

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
	assert(sizeof(uint8) == 1);
	log("Right\n");
	
	log("init: Checking length of uint16... ");
	assert(sizeof(uint16) == 2);
	log("Right\n");
	
	log("init: Checking length of uint32... ");
	assert(sizeof(uint32) == 4);
	log("Right\n");
}

/* This function gets called from loader.asm with the contents of ebx
 * (Which should normally be the pointer to multiboot_info.
 */
void __cdecl kmain(multiboot_info_t *mbootPointer)
{
	init(multiboot, mbootPointer);
	memory_init_preprotected();
	init(interrupts);

	init(kmalloc);
	init(display);
	init(serial);
	init(log);
	
	compilerInfo();
	checkIntLenghts();
	multiboot_printInfo();

	init(argparser, multiboot_info->cmdLine);

	if(mbootPointer->bootLoaderName != NULL && find_substr(multiboot_info->bootLoaderName, "GNU GRUB") != -1)
		init_haveGrub = true;
	else
		log("init: It looks like you don't use GNU GRUB as bootloader. Please note that we only support GRUB and things might be broken.\n");
	
	init(pit, PIT_RATE);
	init(cpu);
	memory_init_postprotected();

	if(mbootPointer->modsCount < 1)
		panic("Could not load initrd (multiboot_info->modsCount < 1).");
	
	
	init(vfs, mbootPointer->modsAddr[0]);

	init(keyboard);
	init(debugconsole);

	asm("sti");
	/* Just in case they're disabled for whatever reason.
	 * Note, just in case that's not obvious:
	 * If you disable interrupts in an interrupt handler and
	 * forget to re-enable them, we're basically screwed, so
	 * be a brave man and don't do that.
	 */
	while(true)
		asm("hlt"); // Wait until interrupt fires
		
	panic("Kernel returned.");
}
