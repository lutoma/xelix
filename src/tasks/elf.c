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
#include <tasks/task.h>
#include <fs/vfs.h>
#include <memory/kmalloc.h>
#include <memory/track.h>
#include <memory/vmem.h>

#ifdef ELF_DEBUG
 #define debug(args...) log(LOG_DEBUG, args);
#else
 #define debug(...)
#endif

#define fail(args...) do { log(LOG_ERR, args); return NULL; } while(false);

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

// Returns the last allocated section's end address so we can set sbrk
int elf_read_sections(elf_t* bin, void* binary_start, uint32_t memsize, task_t* task) {
	elf_section_t* shead = (elf_section_t*)((uint32_t)bin + bin->shoff);

	int i = 0;
	for(; i < bin->shnum; i++) {

		/* Sections without a type are unused/empty. This is usually the case
		 * for the first section.
		 */
		if(!shead->type) {
			goto section_cont;
		}

		#ifdef ELF_DEBUG
			elf_section_t* strings = (elf_section_t*)((uint32_t)bin + bin->shoff + (bin->shentsize * bin->shstrndx));
			char* name = (char*)(uint32_t)bin + strings->offset + shead->name;
			debug("elf: Section %s, type 0x%x, size 0x%x, offset 0x%x\n", name, shead->type, shead->size, shead->offset);
		#endif

		if(shead->flags & SHF_ALLOC) {
			void* dest = (void*)vmem_translate(task->memory_context, (intptr_t)shead->addr, false);
			if(!dest) {
				return -1;
			}

			debug("elf: - Physical addr. 0x%x - 0x%x, virtual 0x%x - 0x%x\n",
				dest, dest + shead->size,
				shead->addr, shead->addr + shead->size);

			if(shead->type != SHT_NOBITS) {
				if(shead->offset + shead->size > memsize) {
					return -1;
				}

				memcpy(dest, (void*)((intptr_t)bin + shead->offset), shead->size);
			}
		}

		section_cont:
		shead = (elf_section_t*)((intptr_t)shead + (intptr_t)bin->shentsize);
	}

	return i;
}

uint32_t alloc_memory(elf_t* bin, void** binary_start, task_t* task) {
	/* Allocates a physical memory region for this binary based on the memory
	 * requirements in the program headers. The binary will then get copied
	 * to that region based on the sections.
	 */

	uint32_t memsize = 0;
	intptr_t phead = (intptr_t)bin + bin->phoff;
	void* pages_start = ((elf_program_header_t*)phead)->vaddr;

	for(int i = 0; i < bin->phnum; i++) {
		memsize += ((elf_program_header_t*)phead)->memsz;
		phead = phead + bin->phentsize;
	}

	memsize = VMEM_ALIGN(memsize) + PAGE_SIZE;
	task->sbrk = pages_start + memsize;
	*binary_start = tmalloc_a(memsize, task);
	vmem_map(task->memory_context, pages_start, *binary_start, memsize, VMEM_SECTION_CODE);

	debug("elf: Allocated physical memory region from 0x%x to 0x%x\n", *binary_start, (intptr_t)*binary_start + memsize);
	return memsize;
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

	if(bin->machine != ELF_ARCH_386)
		fail("elf: elf_load: Attempt to load an elf file for an other architecture\n");

	if(bin->version != ELF_VERSION_CURRENT)
		fail("elf: elf_load: Attempt to load an elf of an unsupported version\n");

	if(bin->entry == NULL)
		fail("elf: elf_load: This elf file doesn't have an entry point\n");

	if(!bin->phnum)
		fail("elf: No program headers\n");

	task_t* task = task_new(bin->entry, NULL, name, environ, envc, argv, argc);

	void* binary_start = NULL;
	uint32_t memsize = alloc_memory(bin, &binary_start, task);
	if(elf_read_sections(bin, binary_start, memsize, task) < 1) {
		fail("elf: Loading sections failed.\n");
	}

	debug("elf: Entry point is 0x%x, sbrk 0x%x\n", bin->entry, task->sbrk);
	return task;
}

task_t* elf_load_file(char* path, char** environ, uint32_t envc, char** argv, uint32_t argc)
{
	vfs_file_t* fd = vfs_open(path, O_RDONLY, NULL);
	if(!fd) {
		return NULL;
	}

	vfs_stat_t* stat = kmalloc(sizeof(vfs_stat_t));
	if(vfs_stat(fd, stat) != 0 || !stat->st_size || !(stat->st_mode & S_IXUSR)) {
		kfree(stat);
		vfs_close(fd);
		return NULL;
	}

	void* data = kmalloc(stat->st_size);
	size_t read = vfs_read(data, stat->st_size, fd);
	kfree(stat);

	if(!read) {
		kfree(data);
		vfs_close(fd);
		return NULL;
	}

	task_t* task = elf_load((elf_t*)data, vfs_basename(path), environ, envc, argv, argc);
	kfree(data);
	vfs_close(fd);
	return task;
}
