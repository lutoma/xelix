/* multiboot.c: Multiboot2 header parsing
 * Copyright © 2018 Lukas Martini
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

#include <multiboot.h>
#include <log.h>
#include <panic.h>

static struct multiboot_tag_mmap* mmap_info = NULL;
static struct multiboot_tag_framebuffer framebuffer_info;

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

struct multiboot_tag_framebuffer* multiboot_get_framebuffer() {
	return &framebuffer_info;
}

void multiboot_init(uint32_t magic, void* header) {
	if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
		panic("Bootloader is not multiboot2 compliant (eax 0x%x != 0x%x).\n",
			magic, MULTIBOOT2_BOOTLOADER_MAGIC);
	}

	// Start of header is 2 uint32_t.
	uint32_t total_size = *(uint32_t*)header;
	intptr_t offset = 8;

	log(LOG_INFO, "multiboot2 tags:\n");
	struct multiboot_tag* tag = header + offset;
	while(tag < (struct multiboot_tag*)(header + total_size)) {
		// Tags are always padded to be 8-aligned
		if((intptr_t)tag % 8) {
			tag = (struct multiboot_tag*)(((intptr_t)tag &~ 7) + 8);
		}

		if(!tag->type || !tag->size) {
			break;
		}

		char* strrep = "";
		if(tag->type == MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME || tag->type == MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME) {
			strrep = (char*)(tag + 1);
		}
		log(LOG_INFO, "  %#-10x size %-4d %-18s %s\n", tag, tag->size, tag_type_names[tag->type], strrep);

		switch(tag->type) {
			case MULTIBOOT_TAG_TYPE_MMAP:
				mmap_info = (struct multiboot_tag_mmap*)tag;
				break;
			case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
				memcpy(&framebuffer_info, (struct multiboot_tag_framebuffer*)tag, sizeof(framebuffer_info));
				break;
		}

		tag = (struct multiboot_tag*)((intptr_t)tag + tag->size);
	}
}
