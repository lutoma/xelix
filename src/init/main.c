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
#include <memory/gdt.h>
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
	log("\tTarget Architecture: %s\n", ARCHNAME);
}

/* This is the very first function of our kernel and gets called
 * directly from the bootloader (GRUB etc.).
 */
void __attribute__((__cdecl__)) _start()
{
	#if ARCH == ARCH_i386 || ARCH == ARCH_amd64
		/* Fetch the pointer to the multiboot_info struct which should be in
		 * EBX.
		 */
		asm("mov %0, ebx" : "=m" (multiboot_info));
		
		// Just some assertions to make sure things are ok.
		assert(multiboot_info != NULL);
	#endif
	
	init(gdt);
	init(interrupts);

	init(kmalloc);
	init(display);
	init(serial);
	init(log);
	
	compilerInfo();
	
	#if ARCH == ARCH_i386 || ARCH == ARCH_amd64
		multiboot_printInfo();
	#endif

	init(argparser, multiboot_info->cmdLine);

	if(multiboot_info->bootLoaderName != NULL && find_substr(multiboot_info->bootLoaderName, "GRUB") != -1)
		init_haveGrub = true;
	else
		log("init: It looks like you don't use GNU GRUB as bootloader. Please note that we only support GRUB and things might be broken.\n");
	
	init(pit, PIT_RATE);
	init(cpu);

	if(multiboot_info->modsCount < 1)
		panic("Could not load initrd (multiboot_info->modsCount < 1).");
	
	
	init(vfs, multiboot_info->modsAddr[0]);

	init(keyboard);
	init(debugconsole);

	// If they were disabled.
	interrupts_enable();
	
	/* And now a comment from our old friend Captain Obvious:
	 * If you disable interrupts in an interrupt handler and
	 * forget to re-enable them, the sky will fall on your head, so
	 * thank you for not doing that.
	 */
	while(true)
		asm("hlt"); // Wait until interrupt fires
		
	panic("Kernel returned.");
}
