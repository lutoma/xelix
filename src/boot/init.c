/* init.c: Initialization code of the kernel
 * Copyright © 2010-2023 Lukas Martini
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
#include <block/block.h>
#include <block/random.h>
#include <cmdline.h>
#include <libgen.h>
#include <tty/serial.h>
#include <int/int.h>
#include <bsp/timer.h>
#include <fs/vfs.h>
#include <tasks/scheduler.h>
#include <gfx/gfx.h>
#include <tty/term.h>
#include <tty/console.h>
#include <bsp/i386-pci.h>
#include <tasks/syscall.h>
#include <tasks/exception.h>
#include <mem/mem.h>
#include <mem/kmalloc.h>
#include <mem/i386-gdt.h>
#include <sound/i386-ac97.h>
#include <boot/multiboot.h>

#ifdef CONFIG_ENABLE_PICOTCP
#include <net/net.h>
#endif

void xelix_main(void);

// Used in lib/errno.h
uint32_t __dummy_errno;

void (*boot_sequence[])(void) = {
#ifdef __i386__
	/* multiboot_init needs to be initialized before paging as it accesses the
     * multiboot header left by the bootloader in an arbitrary location that
     * will not be mapped after paging_init.
     */

	serial_init,  multiboot_init, gdt_init, mem_init, paging_init, int_init, task_exception_init,
	timer_init, mem_late_init, cmdline_init, gfx_init, term_init, time_init, pci_init,
	block_init, vfs_init, timer_init2, task_init
#endif
};

void xelix_main(void) {
	for(int i = 0; i < ARRAY_SIZE(boot_sequence); i++) {
		boot_sequence[i]();
	}

	#ifdef CONFIG_ENABLE_PICOTCP
	net_init();
	#endif

	#ifdef CONFIG_ENABLE_AC97
	ac97_init();
	#endif

	// These only register interrupts or initialize sysfs integration
	syscall_init();
	log_init();
	version_init();
	serial_init2();

	char* init_path = cmdline_get("init");
	if(!init_path) {
		init_path = CONFIG_INIT_PATH;
	}

	log(LOG_INFO, "Starting %s\n", init_path);

	char* __env[] = { NULL };
	char* __argv[] = { basename(init_path), NULL };

	task_t* init = task_new(NULL, 0, init_path, __env, 0, __argv, 1);
	if(!init) {
		panic("Could not spawn init task.\n");
	}

	init->ctty = term_console;
	scheduler_add(init);
	scheduler_init();
}
