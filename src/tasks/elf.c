/* elf.c: Loader for ELF binaries
 * Copyright © 2011 Lukas Martini
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

#define fail(args...) do { log(LOG_INFO, args); return NULL; } while(false);

static char header[4] = {0x7f, 'E', 'L', 'F'};

task_t* elf_load(elf_t* bin, char* name, char** environ, char** argv, int argc)
{
	if(bin <= NULL)
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

	struct vmem_context *ctx = vmem_new();
	elf_program_t* phead = ((void*)bin + bin->phoff);

	for(int i = 0; i < bin->phnum; i++, phead++)
	{
		// Allocate new _physical_ location for this in RAM and copy data there
		void* phys_location = (void*)kmalloc_a(phead->filesize + 4096);

		memset(phys_location, 0, 4096);
		memcpy(phys_location, (void*)bin + phead->offset, phead->filesize);	

		/* Now, remap the _virtual_ location where the ELF binary wants this
		 * section to be at to the physical location.
		 */
		for(int i = 0; i < phead->filesize; i += 4096)
		{
			vmem_rm_page_virt(vmem_kernelContext, phead->virtaddr + i);

			struct vmem_page *page = vmem_new_page();
			page->section = VMEM_SECTION_KERNEL; // FIXME
			page->cow = 0;
			page->allocated = 1;
			page->virt_addr = phead->virtaddr + i;
			page->phys_addr = phys_location + i;
			vmem_add_page(vmem_kernelContext, page);
			vmem_add_page(ctx, page);
		}
	}

	task_t* task = scheduler_new(bin->entry, NULL, name, environ, argv, argc,
		vmem_kernelContext);
	
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