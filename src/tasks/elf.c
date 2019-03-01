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

struct elf_load_ctx {
	int fd;
	task_t* task;
	char* interp;
	char* strtab;
	uint32_t dyndeps[50];
	uint32_t ndyndeps;
};

static char elf_magic[16] = {0x7f, 'E', 'L', 'F', 01, 01, 01, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static inline void* bin_read(int fd, size_t offset, size_t size, void* inbuf) {
	void* buf = inbuf ? inbuf : kmalloc(size);
	vfs_seek(fd, offset, VFS_SEEK_SET, NULL);
	size_t read = vfs_read(fd, buf, size, NULL);
	if(likely(read == size)) {
		return buf;
	}
	if(!inbuf) {
		kfree(buf);
	}
	return NULL;
}

static int load_section(struct elf_load_ctx* ctx, elf_section_t* shead) {
	void* dest = (void*)vmem_translate(ctx->task->memory_context, (intptr_t)shead->addr, false);

	// FIXME: Check for buffer overflow here using size from struct task_mem
	if(unlikely(!dest || !bin_read(ctx->fd, shead->offset, shead->size, dest))) {
		return -1;
	}

	debug("  -> Loaded to phys 0x%x - 0x%x virt 0x%x - 0x%x\n",
		dest, dest + shead->size, shead->addr, shead->addr + shead->size);
	return 0;
}

static int read_sections(elf_t* header, struct elf_load_ctx* ctx) {
	elf_section_t* shead_start = bin_read(ctx->fd, header->shoff, header->shnum * header->shentsize, NULL);
	if(unlikely(!shead_start)) {
		return -1;
	}

	elf_section_t* shead = shead_start;
	int loaded_sections = 0;
	for(int i = 0; i < header->shnum; i++) {
		debug("elf: Section %d, type 0x%x, size 0x%x, offset 0x%x\n",
			i, shead->type, shead->size, shead->offset);

		/* Sections without a type are unused/empty. This is usually the case
		 * for the first section.
		 */
		if(!shead->type) {
			goto section_cont;
		}

		// Load strtab if we need it to resolve dynamic library names
		if(ctx->ndyndeps && shead->type == SHT_STRTAB) {
			ctx->strtab = bin_read(ctx->fd, shead->offset, shead->size, NULL);
			if(!ctx->strtab) {
				return -1;
			}
		}

		if(shead->flags & SHF_ALLOC && shead->type != SHT_NOBITS) {
			if(unlikely(load_section(ctx, shead) < 0)) {
				kfree(shead_start);
				return -1;
			}

			loaded_sections++;
		}

		section_cont:
		shead = (elf_section_t*)((intptr_t)shead + (intptr_t)header->shentsize);
	}

	kfree(shead_start);
	return loaded_sections;
}

static int alloc_phead(struct elf_load_ctx* ctx, elf_program_header_t* phead) {
	size_t size = VMEM_ALIGN(phead->memsz);
	void* virt = VMEM_ALIGN_DOWN(phead->vaddr);
	void* phys = zmalloc_a(size);
	if(unlikely(!phys)) {
		return -1;
	}

	task_add_mem(ctx->task, virt, phys, size, VMEM_SECTION_CODE,
		TASK_MEM_FORK | TASK_MEM_FREE);

	if(virt + size > ctx->task->sbrk) {
		ctx->task->sbrk = virt + size;
	}

	debug("  -> Allocated memory region phys 0x%x-0x%x to virt 0x%x-0x%x\n",
		phys, (intptr_t)phys + size, virt, (intptr_t)virt + size);
	return 0;
}

static uint32_t read_pheads(elf_t* header, struct elf_load_ctx* ctx) {
	elf_program_header_t* phead_start = bin_read(ctx->fd, header->phoff,
		header->phnum * header->phentsize, NULL);
	if(unlikely(!phead_start)) {
		return -1;
	}

	elf_program_header_t* phead = phead_start;

	for(int i = 0; i < header->phnum; i++) {
		debug("elf: Program header %d, type 0x%x, vaddr 0x%x, memsz 0x%x\n",
			i, phead->type, phead->vaddr, phead->memsz);

		switch(phead->type) {
			case PT_LOAD:
				alloc_phead(ctx, phead); break;
			case PT_INTERP:
				ctx->interp = bin_read(ctx->fd, phead->offset, phead->filesz, NULL);
				if(!ctx->interp) {
					return -1;
				}
				break;
			case PT_DYNAMIC:;
				elf_dyn_tag_t* dyn = bin_read(ctx->fd, phead->offset, phead->filesz, NULL);
				for(; dyn->tag; dyn++) {
					if(dyn->tag == DT_NEEDED) {
						ctx->dyndeps[ctx->ndyndeps++] = dyn->val;
					}
				}

				kfree(dyn);
				break;
		}

		phead = (elf_program_header_t*)((intptr_t)phead + header->phentsize);
	}
	kfree(phead_start);
	return 0;
}

#define LF_ASSERT(cmp, msg)                    \
if(unlikely(!(cmp))) {                         \
	log(LOG_INFO, "elf: elf_load: " msg "\n"); \
	vfs_close(ctx->fd, NULL);                  \
	kfree(header);                             \
	sc_errno = ENOEXEC;                        \
	return -1;                                 \
}

int elf_load_file(task_t* task, char* path) {
	char* abs_path = vfs_normalize_path(path, task->cwd);
	struct elf_load_ctx* ctx = zmalloc(sizeof(struct elf_load_ctx));
	ctx->task = task;

	if(!*task->binary_path) {
		strncpy(task->binary_path, abs_path, TASK_PATH_MAX);
	}

	ctx->fd = vfs_open(abs_path, O_RDONLY, NULL);
	kfree(abs_path);
	if(unlikely(ctx->fd < 0)) {
		kfree(ctx);
		return -1;
	}

	elf_t* header = bin_read(ctx->fd, 0, sizeof(elf_t), NULL);
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

	LF_ASSERT(read_pheads(header, ctx) != -1, "Loading program headers failed");

	// If the binary has an interpreter/linker set, run it.
	if(ctx->interp) {
		kfree(header);
		vfs_close(ctx->fd, NULL);
		int r = elf_load_file(task, ctx->interp);
		kfree(ctx->interp);
		kfree(ctx);
		return r;
	}

	int loaded = read_sections(header, ctx);
	LF_ASSERT(loaded > 0, "Loading sections failed.");

	debug("elf: Entry point is 0x%x, sbrk 0x%x\n", header->entry, task->sbrk);
	kfree(header);
	kfree(ctx);
	vfs_close(ctx->fd, NULL);

	return 0;
}
