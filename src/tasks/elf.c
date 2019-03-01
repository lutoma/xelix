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

#define MAXDEPS 50

struct elf_load_ctx {
	int fd;
	task_t* task;
	char* interp;
	void* dynstr_virt;
	uint32_t dyndeps[MAXDEPS];
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

	debug("elf: bin_read: Read size %#x at offset %#x smaller than expected %#x\n", read, offset, size);
	if(!inbuf) {
		kfree(buf);
	}
	return NULL;
}

static int load_phead(struct elf_load_ctx* ctx, elf_program_header_t* phead) {
	int section = TMEM_SECTION_DATA;
	if(phead->flags & PF_X) {
		// Can't be both executable and writable
		if(phead->flags & PF_W) {
			return -1;
		}
		section = TMEM_SECTION_CODE;
	}

	size_t size = VMEM_ALIGN(phead->memsz);
	void* virt = VMEM_ALIGN_DOWN(phead->vaddr);
	void* phys = zmalloc_a(size);
	size_t phys_offset = phead->vaddr - virt;
	if(unlikely(!phys)) {
		return -1;
	}

	if(unlikely(!bin_read(ctx->fd, phead->offset, phead->filesz, phys + phys_offset))) {
		kfree(phys);
		return -1;
	}

	task_add_mem(ctx->task, virt, phys, size, section, TASK_MEM_FORK | TASK_MEM_FREE);
	if(virt + size > ctx->task->sbrk) {
		ctx->task->sbrk = virt + size;
	}

	debug("  phys %#-8x-%#-8x virt %#-8x-%#-8x\n",
		phys, (intptr_t)phys + size, virt, (intptr_t)virt + size);
	return 0;
}

static int read_dyn_table(struct elf_load_ctx* ctx, elf_program_header_t* phead) {
	void* dyn = bin_read(ctx->fd, phead->offset, phead->filesz, NULL);
	if(!dyn) {
		return -1;
	}

	elf_dyn_tag_t* tag = (elf_dyn_tag_t*)dyn;
	while((void*)(tag + 1) < dyn + phead->filesz && tag->tag) {
		if(tag->tag == DT_NEEDED) {
			if(ctx->ndyndeps >= MAXDEPS) {
				return -1;
			}

			ctx->dyndeps[ctx->ndyndeps++] = tag->val;
		}
		if(tag->tag == DT_STRTAB) {
			ctx->dynstr_virt = tag->val;
		}
		tag++;
	}

	kfree(dyn);
}

static uint32_t read_pheads(elf_t* header, struct elf_load_ctx* ctx) {
	elf_program_header_t* phead_start = bin_read(ctx->fd, header->phoff,
		header->phnum * header->phentsize, NULL);
	if(unlikely(!phead_start)) {
		return -1;
	}

	elf_program_header_t* phead = phead_start;

	debug("elf: Program headers:\n");
	for(int i = 0; i < header->phnum; i++) {
		debug("  %-2d type %-2d offset %#-6x vaddr %#-8x memsz %#-8x filesz %#-8x\n",
			i, phead->type, phead->offset, phead->vaddr, phead->memsz, phead->filesz);

		switch(phead->type) {
			case PT_LOAD:
				if(load_phead(ctx, phead) < 0) {
					return -1;
				}
				break;
			case PT_INTERP:
				ctx->interp = bin_read(ctx->fd, phead->offset, phead->filesz, NULL);
				if(!ctx->interp) {
					return -1;
				}
				break;
			case PT_DYNAMIC:
				read_dyn_table(ctx, phead);
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

	if(ctx->ndyndeps) {
		serial_printf("Gathered %d dynamic dependencies\n", ctx->ndyndeps);
		char* dynstr = vmem_translate(task->memory_context, ctx->dynstr_virt, false);

		for(int i = 0; i < ctx->ndyndeps; i++) {
			char* libname = dynstr + ctx->dyndeps[i];
			serial_printf("%d offset %d, lib %s\n", i, ctx->dyndeps[i], libname);
		}
	}

	debug("elf: Entry point is 0x%x, sbrk 0x%x\n", header->entry, task->sbrk);
	kfree(header);
	kfree(ctx);
	vfs_close(ctx->fd, NULL);

	return 0;
}
