/* elf.c: Loader for ELF binaries
 * Copyright © 2011-2016 Lukas Martini
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include "elf.h"

#include <lib/generic.h>
#include <lib/log.h>
#include <lib/print.h>
#include <lib/string.h>
#include <tasks/scheduler.h>
#include <fs/vfs.h>
#include <memory/vmem.h>
#include <memory/kmalloc.h>
#include <memory/track.h>

//#define ELF_DEBUG
#ifdef ELF_DEBUG
 #define debug(args...) log(LOG_DEBUG, args);
#else
 #define debug(...)
#endif

#define fail(args...) do { log(LOG_INFO, args); return NULL; } while(false);

static char header[4] = {0x7f, 'E', 'L', 'F'};

struct vmem_context* map_task(elf_t* bin) {
	/* 1:1 map "lower" memory (kernel etc.)
	 * FIXME This is really generic and hacky. Should instead do some smart
	 * stuff with memory/track.c – That is, as soon as we have a complete
	 * collection of all the memory areas we need.
	 */
	struct vmem_context *ctx = vmem_new();
	for (char *i = (char*)0; i <= (char*)0xfffffff; i += PAGE_SIZE)
	{
		struct vmem_page *page = vmem_new_page();
		page->section = VMEM_SECTION_KERNEL;
		page->cow = 0;
		page->allocated = 1;
		page->virt_addr = (void *)i;
		page->phys_addr = (void *)i;

		vmem_add_page(ctx, page);
	}

	elf_program_t* phead = ((void*)bin + bin->phoff);

	for(int i = 0; i < bin->phnum; i++, phead++)
	{
		if(phead->alignment != PAGE_SIZE && phead->alignment != 0 && phead->alignment != 1) {
			log(LOG_WARN, "elf: Incompatible alignment request (%d). Proceeding, but this may fail.\n",
				phead->alignment);
		}

		debug("Reading program header %d, offset 0x%x, size 0x%x, virt addr 0x%x\n", i, phead->offset, phead->memsize, phead->virtaddr);

		void* phys_location = (void*)bin + phead->offset;

		// Allocate new _physical_ location for this in RAM and copy data there
		/*void* phys_location = (void*)kmalloc_a(phead->memsize + PAGE_SIZE);

		memset(phys_location, 0, phead->memsize + PAGE_SIZE);
		memcpy(phys_location, (void*)bin + phead->offset, phead->filesize + PAGE_SIZE);*/

		bool readonly = !(phead->flags & ELF_PROGRAM_FLAG_WRITE);

		debug("elf: Remapping virt 0x%x to phys 0x%x, size %d (src 0x%x)\n", phead->virtaddr, phys_location, phead->filesize, (void*)bin + phead->offset);

		/* Now, remap the _virtual_ location where the ELF binary wants this
		 * section to be at to the physical location.
		 */
		for(int j = 0; j < phead->memsize; j += PAGE_SIZE)
		{
			debug("elf: - mapping page 0x%x to phys 0x%x\n", phead->virtaddr + j, phys_location + j);

			vmem_rm_page_virt(ctx, phead->virtaddr + j);

			struct vmem_page *page = vmem_new_page();
			page->section = readonly ? VMEM_SECTION_CODE : VMEM_SECTION_HEAP;
			page->readonly = readonly;
			page->cow = 0;
			page->allocated = 1;
			page->virt_addr = phead->virtaddr + j;
			page->phys_addr = phys_location + j;
			vmem_add_page(ctx, page);
		}
	}

	return ctx;
}

task_t* elf_load(elf_t* bin, char* name, char** environ, char** argv, int argc)
{
	if(bin <= (elf_t*)NULL)
		return NULL;

	if(bin->ident.magic[0] != header[0]
	|| bin->ident.magic[1] != header[1]
	|| bin->ident.magic[2] != header[2]
	|| bin->ident.magic[3] != header[3])
		fail("elf: elf_load: Invalid elf header: 0x%x %c%c%c\n",
			bin->ident.magic[0],
			bin->ident.magic[1],
			bin->ident.magic[2],
			bin->ident.magic[3]);

	if(bin->type != ELF_TYPE_EXEC)
		fail("elf: elf_load: Attempt to load an inexecutable elf file\n");

	#if ARCH == ARCH_i386 || ARCH == ARCH_amd64
		if(bin->machine != ELF_ARCH_386)
			fail("elf: elf_load: Attempt to load an elf file for an other architecture\n");
	#endif

	if(bin->version != ELF_VERSION_CURRENT)
		fail("elf: elf_load: Attempt to load an elf of an unsupported version\n");

	if(bin->entry == NULL)
		fail("elf: elf_load: This elf file doesn't have an entry point\n");

	struct vmem_context *ctx = vmem_kernelContext;
	ctx = map_task(bin);

	// The last argument should be false only as long as we use the kernel context
	task_t* task = scheduler_new(bin->entry, NULL, name, environ, argv, argc,
		ctx, false);

	vmem_set_task(ctx, task);
	return task;
}

task_t* elf_load_file(char* path, char** environ, char** argv, int argc)
{
	vfs_file_t* fd = vfs_open(path);
	// Dat dirty hack
	void* data = vfs_read(fd, 9999999);
	if(data == NULL)
		return NULL;
	
	return elf_load(data, path, environ, argv, argc);
}