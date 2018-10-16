/* init.c: Initialization code of the kernel
 * Copyright © 2010-2018 Lukas Martini
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

#include <log.h>
#include <string.h>
#include <panic.h>
#include <time.h>
#include <hw/serial.h>
#include <memory/track.h>
#include <memory/gdt.h>
#include <hw/interrupts.h>
#include <hw/pit.h>
#include <memory/kmalloc.h>
#include <hw/speaker.h>
#include <fs/vfs.h>
#include <tasks/scheduler.h>
#include <console/console.h>
#include <hw/pci.h>
#include <hw/rtl8139.h>
#include <hw/ne2k.h>
#include <tasks/elf.h>
#include <tasks/syscall.h>
#include <memory/paging.h>
#include <memory/vmem.h>
#include <net/slip.h>
#include <hw/ide.h>
#include <fs/part.h>
#include <fs/sysfs.h>
#include <fs/ext2.h>
#include <net/udp.h>
#include <net/echo.h>
#include <hw/ac97.h>
#include <multiboot.h>

void __attribute__((fastcall, noreturn)) xelix_main(uint32_t multiboot_magic,
	void* multiboot_info) {

	init(serial);
	init(multiboot, multiboot_magic, multiboot_info);
	init(memory_track);
	init(kmalloc);
	init(gdt);
	init(interrupts);
	init(pit, PIT_RATE);
	init(console);

	memory_track_print_areas();

	init(vmem);
	init(paging);
	init(time);
	init(pci);
	init(syscall);

	init(ide);
	init(part);
	init(sysfs);
	#ifdef ENABLE_EXT2
	init(ext2);
	#endif
	init(vfs);

	// Networking
	init(udp);
	init(echo);

	#ifdef ENABLE_NE2K
	init(ne2k);
	#endif

	#ifdef ENABLE_RTL8139
	init(rtl8139);
	#endif

	#ifdef XELIX_WITH_SLIP
	init(slip);
	#endif

	#ifdef ENABLE_AC97
	init(ac97);
	#endif

	char* __env[] = { NULL };
	char* __argv[] = { "init", NULL };

	task_t* init = elf_load_file(INIT_PATH, __env, 0, __argv, 1);
	if(!init) {
		panic("Could not start init (Tried " INIT_PATH ").\n");
	}
	scheduler_add(init);
	scheduler_init();

	asm(
	".il:"
		"hlt;"
		"jmp .il;"
		"ud2;"
		"cli;"
	);
	__builtin_unreachable();
}
