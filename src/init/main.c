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
#if ARCH == ARCH_i386 || ARCH == ARCH_amd64
	#include <arch/i386/lib/multiboot.h>
	#include <arch/i386/lib/acpi.h>
#endif
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
#include <fs/vfs.h>
#include <fs/memfs/interface.h>
#include <tasks/task.h>
#include <lib/argparser.h>
#include <tasks/scheduler.h>
#include <init/debugconsole.h>
#include <console/interface.h>
#include <hw/pci.h>
#include <hw/rtl8139.h>

// Prints out compiler information, especially for GNU GCC
static void compilerInfo()
{
	log("Xelix %d.%d.%d%s (Build %d)\n", VERSION, VERSION_MINOR, VERSION_PATCHLEVEL, VERSION_APPENDIX, BUILD);
	log("\tCompiled at: %s %s\n", __DATE__, __TIME__);
	// Test for GCC > 3.2.0
	#if GCC_VERSION > 30200
		log("\tCompiler: GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
	#else
		log("\tCompiler: GCC (< 3.2.0)\n");
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
	init(keyboard);
	init(serial);
	init(console);
	init(log);
	
	compilerInfo();
	
	#if ARCH == ARCH_i386 || ARCH == ARCH_amd64
		arch_multiboot_printInfo();
	#endif

	init(argparser, multiboot_info->cmdLine);

	if(multiboot_info->bootLoaderName != NULL && find_substr(multiboot_info->bootLoaderName, "GRUB") != -1)
		init_haveGrub = true;
	else
		log("init: It looks like you don't use GNU GRUB as bootloader. Please note that we only support GRUB and things might be broken.\n");
	
	init(pit, PIT_RATE);
	init(cpu);
	init(acpi);

	
	init(vfs);
	init(pci);
	init(rtl8139);

	init(debugconsole);

	init(scheduler); // Intentionally last

	/* And now a comment from our old friend Captain Obvious:
	 * If you disable interrupts in an interrupt handler and
	 * forget to re-enable them, the sky will fall on your head, so
	 * thank you for not doing that.
	 */
	while(true)
		asm("hlt"); // Wait until interrupt fires
		
	panic("Kernel returned.");
}
