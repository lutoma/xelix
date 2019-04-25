#pragma once

/* Copyright © 2011-2018 Lukas Martini
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <tasks/scheduler.h>

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

#define DT_NULL 0
#define DT_NEEDED 1
#define DT_PLTRELSZ 2
#define DT_PLTGOT 3
#define DT_HASH 4
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9
#define DT_STRSZ 10
#define DT_SYMENT 11
#define DT_INIT 12
#define DT_FINI 13
#define DT_SONAME 14
#define DT_RPATH 15
#define DT_SYMBOLIC 16
#define DT_REL 17
#define DT_RELSZ 18
#define DT_RELENT 19
#define DT_PLTREL 20

#define PF_X		(1 << 0)
#define PF_W		(1 << 1)
#define PF_R		(1 << 2)

#define ELF_TYPE_NONE 0
#define ELF_TYPE_REL 1
#define ELF_TYPE_EXEC 2
#define ELF_TYPE_DYN 3
#define ELF_TYPE_CORE 4

#define ELF_ARCH_NONE 0
#define ELF_ARCH_386 3

#define ELF_VERSION_CURRENT 1

typedef struct {
	uint8_t		ident[16];
	uint16_t	type;		/* Object file type */
	uint16_t	machine;	/* Architecture */
	uint32_t	version;	/* Object file version */
	void*		entry;		/* Entry point virtual address */
	uint32_t	phoff;		/* Program header table file offset */
	uint32_t	shoff;		/* Section header table file offset */
	uint32_t	flags;		/* Processor-specific flags */
	uint16_t	ehsize;		/* ELF header size in bytes */
	uint16_t	phentsize;	/* Program header table entry size */
	uint16_t	phnum;		/* Program header table entry count */
	uint16_t	shentsize;	/* Section header table entry size */
	uint16_t	shnum;		/* Section header table entry count */
	uint16_t	shstrndx;	/* Section header string table index */
} __attribute__((packed)) elf_t;

typedef struct {
	uint32_t type;
	uint32_t offset;
	void* vaddr;
	void* paddr;
	uint32_t filesz;
	uint32_t memsz;
	uint32_t flags;
	uint32_t align;
} __attribute__((packed)) elf_program_header_t;

struct elf_section {
	uint32_t name;
	uint32_t type;
	uint32_t flags;
	void* addr;
	uint32_t offset;
	uint32_t size;
	uint32_t link;
	uint32_t info;
	uint32_t addralign;
	uint32_t entsize;
};

struct elf_sym {
	uint32_t name;
	uint32_t value;
	uint32_t size;
	uint8_t	info;
	uint8_t	other;
	uint16_t shndx;
};

typedef struct {
	int32_t tag;
	int32_t val;
} elf_dyn_tag_t;

int elf_load_file(task_t* task, char* path);
