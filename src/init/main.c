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
#include <lib/panic.h>
#include <hw/display.h>
#include <hw/serial.h>
#include <hw/cpu.h>
#include <memory/interface.h>
#include <memory/gdt.h>
#include <interrupts/interface.h>
#include <hw/pit.h>
#include <memory/kmalloc.h>
#include <hw/speaker.h>
#include <fs/vfs.h>
#include <fs/xsfs.h>
#include <lib/argparser.h>
#include <tasks/scheduler.h>
#include <console/interface.h>
#include <hw/pci.h>
#include <hw/rtl8139.h>
#include <tasks/elf.h>
#include <tasks/syscall.h>
#include <memory/paging.h>
#include <memory/vmem.h>
#include <net/slip.h>
#include <hw/ata.h>

// Prints out compiler information, especially for GNU GCC
static void compilerInfo()
{
	log(LOG_INFO, "Xelix %d.%d.%d%s (Build %d)\n", VERSION, VERSION_MINOR, VERSION_PATCHLEVEL, VERSION_APPENDIX, BUILD);
	log(LOG_INFO, "\tCompiled at: %s %s\n", __DATE__, __TIME__);
	#ifdef __GNUC__
		// Test for GCC > 3.2.0
		#if GCC_VERSION > 30200
			log(LOG_INFO, "\tCompiler: GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
		#else
			log(LOG_INFO, "\tCompiler: GCC (< 3.2.0)\n");
		#endif
	#endif
	log(LOG_INFO, "\tBy: %s\n", __BUILDCOMP__);
	log(LOG_INFO, "\tOS: %s\n", __BUILDSYS__);
	log(LOG_INFO, "\tTarget Architecture: %s\n", ARCHNAME);
}

/* This is the very first function of our kernel and gets called
 * directly from the bootloader (GRUB etc.).
 */
void __attribute__((__cdecl__)) main(multiboot_info_t* mBoot)
{
	multiboot_info = mBoot;

	init(gdt);
	init(interrupts);
	init(panic);
	init(kmalloc);
	init(pit, PIT_RATE);
	init(serial);
	init(console);
	init(log);
	
	compilerInfo();
	
	#if ARCH == ARCH_i386 || ARCH == ARCH_amd64
		arch_multiboot_printInfo();
	#endif

	init(argparser, multiboot_info->cmdLine);
	init(cpu);
	init(acpi);	
	init(pci);
	init(syscall);
	init(vmem);
	init(paging);
	init(ata);

	// TODO remove hardcoded stuff
	vfs_mount("/", &xsfs_read);

	// Networking
	init(rtl8139);
	#ifndef XELIX_WITHOUT_SLIP
		init(slip);
	#endif

	if(multiboot_info->modsCount)
			scheduler_add(scheduler_newTask(elf_load((void*)multiboot_info->modsAddr[0].start), NULL, "initrd"));

	void* data = elf_load_file("/init");
	if(data)
		scheduler_add(scheduler_newTask(data, NULL, "/init"));

	/* Is intentionally last. It's also intentional that the init()
	 * macro isn't used here. Seriously, don't mess around here.
	 */
	scheduler_init();

	/* And now a comment from our old friend Captain Obvious:
	 * If you disable interrupts in an interrupt handler and
	 * forget to re-enable them, the sky will fall on your head, so
	 * thank you for not doing that.
	 */
	while(true)
		asm("hlt"); // Wait until interrupt fires
		
	panic("Kernel returned.");
}
