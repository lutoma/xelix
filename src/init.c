/* init.c: Initialization code of the kernel
 * Copyright © 2010-2015 Lukas Martini
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

#include <lib/multiboot.h>
#include <lib/log.h>
#include <lib/string.h>
#include <lib/panic.h>
#include <hw/serial.h>
#include <hw/cpu.h>
#include <memory/interface.h>
#include <memory/track.h>
#include <memory/gdt.h>
#include <hw/interrupts.h>
#include <hw/pit.h>
#include <memory/kmalloc.h>
#include <hw/speaker.h>
#include <fs/vfs.h>
#include <tasks/scheduler.h>
#include <console/interface.h>
#include <hw/pci.h>
#include <hw/rtl8139.h>
#include <tasks/elf.h>
#include <tasks/syscall.h>
#include <memory/paging.h>
#include <memory/vmem.h>
#include <net/slip.h>
#include <hw/ide.h>
#include <fs/ext2.h>
#include <fs/xsfs.h>
#include <net/udp.h>
#include <net/echo.h>
#include <hw/ac97.h>

// Prints out compiler information, especially for GNU GCC
static void compiler_info()
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
void __attribute__((__cdecl__)) main(uint32_t multiboot_checksum, multiboot_info_t* mBoot)
{
	multiboot_info = mBoot;

	init(gdt);
	init(interrupts);
	init(panic);
	init(cpu);

	// Check if we were actually booted by a multiboot bootloader
	if(multiboot_checksum != MULTIBOOT_KERNELMAGIC) {
		panic("Was not booted by a multiboot compliant bootloader.\n");
	}

	// Find out if we have enough memory to safely operate
	if(!bit_get(multiboot_info->flags, 1)) {
		panic("No memory information passed by bootloader.\n");
	}

	if((multiboot_info->memLower + multiboot_info->memUpper) < (60 * 1024)) {
		panic("Not enough RAM to safely proceed - should be at least 60 MB.\n");
	}

	if(!bit_get(multiboot_info->flags, 6)) {
		panic("No mmap data from bootloader.\n");
	}

	init(memory_track, multiboot_info);
	init(kmalloc);
	init(pit, PIT_RATE);
	init(serial);
	init(console);
	init(log);

	compiler_info();
	memory_track_print_areas();

	#if ARCH == ARCH_i386 || ARCH == ARCH_amd64
		arch_multiboot_printInfo();
	#endif

	init(pci);
	init(syscall);
	init(vmem);
	init(paging);
	init(ide);

	init(ext2);
	init(xsfs);

	// Networking
	init(udp);
	init(echo);
	init(rtl8139);
	#ifndef XELIX_WITHOUT_SLIP
		init(slip);
	#endif

	init(ac97);

	char* __env[] = { NULL };
	char* __argv[] = { "init", NULL };

	task_t* init = elf_load_file("/xinit", __env, __argv, 2);
	if(!init) {
		panic("Could not start init (Tried /xinit).\n");
	}

	scheduler_add(init);

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
