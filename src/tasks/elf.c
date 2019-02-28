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

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PPHEAD 6
#define PT_HISUNW 0x6fffffff
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

#define ELF_TYPE_NONE 0
#define ELF_TYPE_REL 1
#define ELF_TYPE_EXEC 2
#define ELF_TYPE_DYN 3
#define ELF_TYPE_CORE 4

#define ELF_ARCH_NONE 0
#define ELF_ARCH_386 3

#define ELF_VERSION_CURRENT 1

static char elf_magic[16] = {0x7f, 'E', 'L', 'F', 01, 01, 01, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static inline void* bin_read(int fd, task_t* task, size_t offset, size_t size, void* inbuf) {
	void* buf = inbuf ? inbuf : kmalloc(size);
	vfs_seek(fd, offset, VFS_SEEK_SET, task->parent);
	size_t read = vfs_read(fd, buf, size, task->parent);
	if(likely(read == size)) {
		return buf;
	}
	if(!inbuf) {
		kfree(buf);
	}
	return NULL;
}

static int read_sections(int fd, elf_t* header, void* binary_start, uint32_t memsize, task_t* task) {
	elf_section_t* shead_start = bin_read(fd, task, header->shoff,
		header->shnum * header->shentsize, NULL);
	if(!shead_start) {
		return -1;
	}

	elf_section_t* shead = shead_start;
	int loaded_sections = 0;
	for(int i = 0; i < header->shnum; i++) {
		debug("elf: Section %d, type 0x%x, size 0x%x, offset 0x%x\n",
			i, shead->type, shead->size, shead->offset);

		/* Sections without a type are unused/empty. This is usually the case
		 * for the first section. We also only care about loadable sections.
		 */
		if(!shead->type || shead->type == SHT_NOBITS || !(shead->flags & SHF_ALLOC)) {
			goto section_cont;
		}

		void* dest = (void*)vmem_translate(task->memory_context, (intptr_t)shead->addr, false);
		if(unlikely(!dest || shead->offset + shead->size > memsize ||
			!bin_read(fd, task, shead->offset, shead->size, dest))) {

			kfree(shead_start);
			return -1;
		}

		loaded_sections++;
		debug("elf: Mapped from phys 0x%x - 0x%x to virt 0x%x - 0x%x\n",
			dest, dest + shead->size, shead->addr, shead->addr + shead->size);

		section_cont:
		shead = (elf_section_t*)((intptr_t)shead + (intptr_t)header->shentsize);
	}

	kfree(shead_start);
	return loaded_sections;
}

static uint32_t read_pheads(int fd, elf_t* header, void** binary_start, char** interp, task_t* task) {
	/* Allocates a physical memory region for this binary based on the memory
	 * requirements in the program headers. The binary will then get copied
	 * to that region based on the sections.
	 */

	elf_program_header_t* phead_start = bin_read(fd, task, header->phoff,
		header->phnum * header->phentsize, NULL);
	if(unlikely(!phead_start)) {
		return -1;
	}

	uint32_t memsize = 0;
	elf_program_header_t* phead = phead_start;
	void* pages_start = phead->vaddr;

	for(int i = 0; i < header->phnum; i++) {
		switch(phead->type) {
			case PT_LOAD:
				memsize += phead->memsz; break;
			case PT_INTERP:;
				*interp = bin_read(fd, task, phead->offset, phead->filesz, NULL);
				return 0;
		}

		phead = (elf_program_header_t*)((intptr_t)phead + header->phentsize);
	}
	kfree(phead_start);

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
	if(unlikely(fd == -1)) {
		return -1;
	}

	elf_t* header = bin_read(fd, task, 0, sizeof(elf_t), NULL);
	LF_ASSERT(header, "Could not read ELF header");
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
	char* interp = NULL;
	uint32_t memsize = read_pheads(fd, header, &binary_start, &interp, task);
	LF_ASSERT((memsize > 0 && binary_start) || interp, "Loading program headers failed.");

	// If we the binary has an interpreter/linker set, run it.
	if(interp) {
		kfree(header);
		vfs_close(fd, task->parent);
		int r = elf_load_file(task, interp);
		kfree(interp);
		return r;
	}

	int loaded = read_sections(fd, header, binary_start, memsize, task);
	LF_ASSERT(loaded > 0, "Loading sections failed.");

	debug("elf: Entry point is 0x%x, sbrk 0x%x\n", header->entry, task->sbrk);
	kfree(header);
	vfs_close(fd, task->parent);
	return 0;
}
