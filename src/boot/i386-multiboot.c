/* multiboot.c: Multiboot2 header parsing
 * Copyright © 2018-2019 Lukas Martini
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

#include <boot/multiboot.h>
#include <log.h>
#include <printf.h>
#include <panic.h>
#include <tasks/elf.h>

static struct multiboot_tag_mmap* mmap_info = NULL;
static struct multiboot_tag_basic_meminfo* mem_info = NULL;
static struct multiboot_tag_framebuffer framebuffer_info;

/* These are set by i386-boot.asm right after boot */
uint32_t multiboot_magic;
void* multiboot_header;

/* Optimally, this would kmalloc based on ELF sizes, but this
 * is loaded before kmalloc is ready, and afterwards the data
 * may have been overwritten. Instead, allocate static buffer
 * and hope it fits.
 */
#define SYMTAB_BSIZE 0x10000
static char cmdline[0x400];
static char symtab[SYMTAB_BSIZE];
static char strtab[SYMTAB_BSIZE];
size_t symtab_len = 0;
size_t strtab_len = 0;

static char* tag_type_names[] = {
	NULL,
	"cmdline",			// 1
	"boot_loader_name", // 2
	"modules",			// 3
	"basic_mem",		// 4
	"bios_boot_dev",	// 5
	"memory_map",		// 6
	"vbe",				// 7
	"framebuffer",		// 8
	"elf_symbols",		// 9
	"apm",				// 10
	"efi32_sys_table",	// 11
	"efi64_sys_table",	// 12
	"smbios",			// 13
	"acpi1_rsdp",		// 14
	"acpi2_rsdp",		// 15
	"netinfo",			// 16
	"efi_mmap",			// 17
	"efi_boot_nterm",	// 18
	"efi32_image_hdl",	// 19
	"efi64_image_hdl",	// 20
	"image_phys_addr"	// 21
};

struct multiboot_tag_mmap* multiboot_get_mmap() {
	return mmap_info;
}

struct multiboot_tag_basic_meminfo* multiboot_get_meminfo() {
	return mem_info;
}

struct multiboot_tag_framebuffer* multiboot_get_framebuffer() {
	return &framebuffer_info;
}

struct elf_sym* multiboot_get_symtab(size_t* length) {
	*length = symtab_len;
	return (struct elf_sym*)&symtab;
}

char* multiboot_get_strtab(size_t* length) {
	*length = strtab_len;
	return strtab;
}

char* multiboot_get_cmdline() {
	return cmdline;
}

static int extract_symtab(struct multiboot_tag_elf_sections* multiboot_tag) {
	int r = -2;
	struct elf_section* elf_section = (struct elf_section*)multiboot_tag->sections;
	struct elf_section* sh_section = (struct elf_section*)((intptr_t)elf_section
		+ (intptr_t)multiboot_tag->entsize
		* (intptr_t)multiboot_tag->shndx);

	for(int i = 0; i < multiboot_tag->num; i++) {
		char* shname = sh_section->addr + elf_section->name;

		if(!strcmp(".symtab", shname)) {
			symtab_len = MIN(SYMTAB_BSIZE, elf_section->size);
			memcpy(&symtab, elf_section->addr, symtab_len);
			r++;
		}
		if(!strcmp(".strtab", shname)) {
			strtab_len = MIN(SYMTAB_BSIZE, elf_section->size);
			memcpy(&strtab, elf_section->addr, strtab_len);
			r++;
		}
		elf_section = (struct elf_section*)((intptr_t)elf_section + (intptr_t)multiboot_tag->entsize);
	}
	return r;
}

void multiboot_init() {
	if(multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
		panic("Bootloader is not multiboot2 compliant (eax 0x%x != 0x%x).\n",
			multiboot_magic, MULTIBOOT2_BOOTLOADER_MAGIC);
	}

	// Start of header is 2 uint32_t.
	uint32_t total_size = *(uint32_t*)multiboot_header;
	intptr_t offset = 8;

	log(LOG_INFO, "multiboot2 tags:\n");
	struct multiboot_tag* tag = multiboot_header + offset;
	char strrep[150];

	while(tag < (struct multiboot_tag*)(multiboot_header + total_size)) {
		// Tags are always padded to be 8-aligned
		tag = ALIGN(tag, 8);

		if(!tag->type || !tag->size) {
			break;
		}

		switch(tag->type) {
			case MULTIBOOT_TAG_TYPE_CMDLINE:
				strncpy(cmdline, (char*)(tag + 1), ARRAY_SIZE(cmdline) - 1);
				// intentional fallthrough
			case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
				strncpy(strrep, (char*)(tag + 1), 149);
				break;
			case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR:
				snprintf(strrep, 150, "%#x", ((struct multiboot_tag_load_base_addr*)tag)->load_base_addr);
				break;
			case MULTIBOOT_TAG_TYPE_MMAP:
				/* This can be passed by reference since it's only used before
				 * memory system initialization and won't be erased earlier.
				 */
				mmap_info = (struct multiboot_tag_mmap*)tag;
				break;
			case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
				// see above
				mem_info = (struct multiboot_tag_basic_meminfo*)tag;
				snprintf(strrep, 150, "lower=%d upper=%d", mem_info->mem_lower, mem_info->mem_upper);
				break;
			case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
				if(extract_symtab((struct multiboot_tag_elf_sections*)tag) == 0) {
					snprintf(strrep, 150, "symtab=%p, strtab=%p", &symtab, &strtab);
				}
				break;
			case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
				memcpy(&framebuffer_info, tag, sizeof(framebuffer_info));
				break;
		}

		log(LOG_INFO, "  %#p size %-4d %-18s %s\n", tag, tag->size, tag_type_names[tag->type], strrep);
		tag = (struct multiboot_tag*)((intptr_t)tag + tag->size);
		*strrep = 0;
	}
}
