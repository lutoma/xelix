/* elf.c: Loader for ELF binaries
 * Copyright © 2011-2018 Lukas Martini
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

#include <log.h>
#include <print.h>
#include <string.h>
#include <tasks/scheduler.h>
#include <fs/vfs.h>
#include <memory/vmem.h>
#include <memory/kmalloc.h>
#include <memory/track.h>

#ifdef ELF_DEBUG
 #define debug(args...) log(LOG_DEBUG, args);
#else
 #define debug(...)
#endif

#define fail(args...) do { log(LOG_INFO, args); return NULL; } while(false);

#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MASKPROC 0xf0000000

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xffffffff

#define ELF_TYPE_NONE 0
#define ELF_TYPE_REL 1
#define ELF_TYPE_EXEC 2
#define ELF_TYPE_DYN 3
#define ELF_TYPE_CORE 4

#define ELF_ARCH_NONE 0
#define ELF_ARCH_386 3

#define ELF_VERSION_CURRENT 1

static char header[4] = {0x7f, 'E', 'L', 'F'};

static void map_section(elf_t* bin, struct vmem_context* ctx, elf_section_t* shead) {
	void* data = (void*)((intptr_t)bin + shead->offset);
	debug("Allocating block from phys 0x%x -> 0x%x to virt 0x%x -> 0x%x\n",
		data, data + shead->size, shead->addr, shead->addr + shead->size);

	uint32_t pages_start = (uint32_t)VMEM_ALIGN_DOWN(shead->addr);
	uint32_t phys_start = (uint32_t)VMEM_ALIGN_DOWN(data);
	bool readonly = !(shead->flags & SHF_WRITE);

	// A bit hacky, should probably detect this based on SHF_EXECINSTR
	int page_section = VMEM_SECTION_HEAP;
	if(shead->type == SHT_PROGBITS && readonly) {
		page_section = VMEM_SECTION_CODE;
	}

	debug("Pages start: 0x%x, phys start: 0x%x\n", pages_start, phys_start);
	for(int j = 0; j < VMEM_ALIGN(shead->size); j += PAGE_SIZE)
	{
		debug("elf: - mapping page 0x%x to phys 0x%x\n", pages_start + j, phys_start + j);

		struct vmem_page* opage = vmem_get_page_virt(ctx, (void*)(pages_start + j));
		if(opage) {
			vmem_rm_page_virt(ctx, (void*)(pages_start + j));
		}

		struct vmem_page *page = vmem_new_page();
		page->section = page_section;
		page->readonly = readonly;
		page->cow = 0;
		page->allocated = 1;
		page->virt_addr = (void*)(pages_start + j);
		page->phys_addr = (void*)(phys_start + j);
		vmem_add_page(ctx, page);
	}
}

// Returns the last allocated section's end address so we can set sbrk
void* elf_read_sections(elf_t* bin, struct vmem_context* ctx) {
	elf_section_t* shead = (elf_section_t*)((uint32_t)bin + bin->shoff);
	elf_section_t* last = NULL;

	for(int i = 0; i < bin->shnum; i++) {

		/* Sections without a type are unused/empty. This is usually the case
		 * for the first section.
		 */
		if(!shead->type) {
			goto section_cont;
		}

		#ifdef ELF_DEBUG
			elf_section_t* strings = (elf_section_t*)((uint32_t)bin + bin->shoff + (bin->shentsize * bin->shstrndx));
			char* name = (char*)(uint32_t)bin + strings->offset + shead->name;
			debug("elf: Section name %s, type 0x%x, addr 0x%x, size 0x%x\n", name, shead->type, shead->addr, shead->size);
		#endif

		if(ctx && (shead->flags & SHF_ALLOC)) {
			map_section(bin, ctx, shead);
			last = shead;
		}

		section_cont:
		shead = (elf_section_t*)((intptr_t)shead + (intptr_t)bin->shentsize);
	}

	return last ? (void*)VMEM_ALIGN((intptr_t)last->addr + last->size) : NULL;
}

task_t* elf_load(elf_t* bin, char* name, char** environ, uint32_t envc, char** argv, uint32_t argc)
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

	if(!bin->phnum)
		fail("elf: No program headers\n");

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
		page->readonly = 1;
		page->virt_addr = (void *)i;
		page->phys_addr = (void *)i;

		vmem_add_page(ctx, page);
	}

	task_t* task = scheduler_new(bin->entry, NULL, name, environ, envc, argv,
		argc, ctx, true);

	task->sbrk = elf_read_sections(bin, ctx);
	debug("Entry point is 0x%x, sbrk 0x%x\n", bin->entry, task->sbrk);

	vmem_set_task(ctx, task);
	task->binary_start = bin;

	return task;
}

task_t* elf_load_file(char* path, char** environ, uint32_t envc, char** argv, uint32_t argc)
{
	vfs_file_t* fd = vfs_open(path);
	if(!fd) {
		return NULL;
	}

	vfs_stat_t* stat = kmalloc(sizeof(vfs_stat_t));
	if(vfs_stat(fd, stat) != 0 || !stat->st_size) {
		kfree(stat);
		return NULL;
	}

	void* data = kmalloc_a(stat->st_size);
	size_t read = vfs_read(data, stat->st_size, fd);
	kfree(stat);

	if(!read) {
		kfree(data);
		return NULL;
	}

	return elf_load(data, path, environ, envc, argv, argc);
}
