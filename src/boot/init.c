/* init.c: Initialization code of the kernel
 * Copyright © 2010-2019 Lukas Martini
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
#include <panic.h>
#include <time.h>
#include <version.h>
#include <hw/serial.h>
#include <hw/interrupts.h>
#include <hw/pit.h>
#include <fs/vfs.h>
#include <tasks/scheduler.h>
#include <tty/tty.h>
#include <hw/pci.h>
#include <tasks/elf.h>
#include <tasks/syscall.h>
#include <mem/mem.h>
#include <mem/gdt.h>
#include <hw/ac97.h>
#include <multiboot.h>

#ifdef ENABLE_PICOTCP
#include <net/net.h>
#endif

void __fastcall xelix_main(uint32_t multiboot_magic, void* multiboot_info) {
	serial_init();

	#ifdef __i386__
	gdt_init();
	#endif
	interrupts_init();
	pit_init();
	#ifdef __i386__
	multiboot_init(multiboot_magic, multiboot_info);
	#endif
	mem_init();
	tty_init();
	time_init();
	#ifdef __i386__
	pci_init();
	#endif
	vfs_init();
	#ifdef __i386__
	pit_init2();
	#endif

	#ifdef ENABLE_PICOTCP
	net_init();
	#endif

	#ifdef ENABLE_AC97
	ac97_init();
	#endif

	// These only register interrupts or initialize sysfs integration
	syscall_init();
	log_init();
	version_init();

	#if __i386__

	char* __env[] = { NULL };
	char* __argv[] = { vfs_basename(INIT_PATH), NULL };

	task_t* init = task_new(NULL, 0, INIT_PATH, __env, 0, __argv, 1);
	if(elf_load_file(init, INIT_PATH) == -1) {
		panic("Could not start init (Tried " INIT_PATH ").\n");
	}
	scheduler_add(init);
	scheduler_init();
	#endif
}
