/* elf.c: Loader for ELF binaries
 * Copyright © 2011-2019 Lukas Martini
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
#include <errno.h>
#include <tasks/task.h>
#include <fs/vfs.h>
#include <mem/kmalloc.h>
#include <mem/track.h>
#include <mem/vmem.h>

#ifdef ELF_DEBUG
 #define debug(args...) log(LOG_DEBUG, args);
#else
 #define debug(...)
#endif

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

static char elf_magic[16] = {0x7f, 'E', 'L', 'F', 01, 01, 01, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static int read_sections(int fd, elf_t* header, void* binary_start, uint32_t memsize, task_t* task) {
	elf_section_t* section_headers = kmalloc(header->shnum * header->shentsize);
	vfs_seek(fd, header->shoff, VFS_SEEK_SET, task->parent);

	size_t read = vfs_read(fd, section_headers, header->shnum * header->shentsize, task->parent);
	if(read != header->shnum * header->shentsize) {
		kfree(section_headers);
		return -1;
	}

	elf_section_t* shead = section_headers;
	int loaded_sections = 0;
	for(int i = 0; i < header->shnum; i++) {

		/* Sections without a type are unused/empty. This is usually the case
		 * for the first section.
		 */
		if(!shead->type) {
			goto section_cont;
		}

		debug("elf: Section %d, type 0x%x, size 0x%x, offset 0x%x\n",
			i, shead->type, shead->size, shead->offset);

		// We only care about sections we have to load here
		if(shead->type == SHT_NOBITS || !(shead->flags & SHF_ALLOC)) {
			goto section_cont;
		}

		void* dest = (void*)vmem_translate(task->memory_context, (intptr_t)shead->addr, false);
		if(!dest) {
			kfree(section_headers);
			return -1;
		}

		debug("elf: - Physical addr. 0x%x - 0x%x, virtual 0x%x - 0x%x\n",
			dest, dest + shead->size,
			shead->addr, shead->addr + shead->size);

		if(shead->offset + shead->size > memsize) {
			kfree(section_headers);
			return -1;
		}

		vfs_seek(fd, shead->offset, VFS_SEEK_SET, task->parent);
		size_t read = vfs_read(fd, dest, shead->size, task->parent);
		if(read != shead->size) {
			kfree(section_headers);
			return -1;
		}

		loaded_sections++;

		section_cont:
		shead = (elf_section_t*)((intptr_t)shead + (intptr_t)header->shentsize);
	}

	kfree(section_headers);
	return loaded_sections;
}

static uint32_t alloc_memory(int fd, elf_t* header, void** binary_start, task_t* task) {
	/* Allocates a physical memory region for this binary based on the memory
	 * requirements in the program headers. The binary will then get copied
	 * to that region based on the sections.
	 */

	void* program_headers = kmalloc(header->phnum * header->phentsize);
	vfs_seek(fd, header->phoff, VFS_SEEK_SET, task->parent);
	size_t read = vfs_read(fd, program_headers, header->phnum * header->phentsize, task->parent);
	if(read != header->phnum * header->phentsize) {
		kfree(program_headers);
		return -1;
	}

	uint32_t memsize = 0;
	intptr_t phead = (intptr_t)program_headers;
	void* pages_start = ((elf_program_header_t*)phead)->vaddr;

	for(int i = 0; i < header->phnum; i++) {
		memsize += ((elf_program_header_t*)phead)->memsz;
		phead = phead + header->phentsize;
	}
	kfree(program_headers);

	memsize = VMEM_ALIGN(memsize) + PAGE_SIZE;
	task->sbrk = pages_start + memsize;
	*binary_start = zmalloc_a(memsize);

	task_add_mem(task, pages_start, *binary_start, memsize, VMEM_SECTION_CODE,
		TASK_MEM_FORK | TASK_MEM_FREE);

	debug("elf: Allocated physical memory region from 0x%x to 0x%x\n",
		*binary_start, (intptr_t)*binary_start + memsize);

	return memsize;
}

#define LF_ASSERT(cmp, msg)                    \
if(unlikely(!(cmp))) {                         \
	log(LOG_INFO, "elf: elf_load: " msg "\n"); \
	vfs_close(fd, task->parent);               \
	kfree(header);                             \
	sc_errno = ENOEXEC;						   \
	return -1;                                 \
}

int elf_load_file(task_t* task, char* path) {
	// Need to use task->parent to resolve relative paths from excecve
	int fd = vfs_open(path, O_RDONLY, task->parent);
	if(fd == -1) {
		return -1;
	}

	elf_t* header = kmalloc(sizeof(elf_t));
	size_t read = vfs_read(fd, (void*)header, sizeof(elf_t), task->parent);
	LF_ASSERT(read == sizeof(elf_t), "Could not read ELF header");
	LF_ASSERT(!memcmp(header->ident, elf_magic, sizeof(elf_magic)),
		"Invalid magic");

	LF_ASSERT(header->type == ELF_TYPE_EXEC, "Binary is inexecutable");
	LF_ASSERT(header->machine == ELF_ARCH_386, "Invalid architecture");
	LF_ASSERT(header->version == ELF_VERSION_CURRENT, "Unsupported ELF version");
	LF_ASSERT(header->entry, "Binary has no entry point");
	LF_ASSERT(header->phnum, "No program headers");
	LF_ASSERT(header->shnum, "No section headers");
	task_set_initial_state(task, header->entry);

	void* binary_start = NULL;
	uint32_t memsize = alloc_memory(fd, header, &binary_start, task);
	LF_ASSERT(memsize > 0 && binary_start, "Loading program headers failed.");

	int loaded = read_sections(fd, header, binary_start, memsize, task);
	LF_ASSERT(loaded > 0, "Loading sections failed.");

	debug("elf: Entry point is 0x%x, sbrk 0x%x\n", header->entry, task->sbrk);
	kfree(header);
	vfs_close(fd, task->parent);
	return 0;
}
