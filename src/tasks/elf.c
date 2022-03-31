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
#include <printf.h>
#include <string.h>
#include <errno.h>
#include <tasks/task.h>
#include <fs/vfs.h>
#include <mem/kmalloc.h>
#include <mem/mem.h>

#ifdef CONFIG_ELF_DEBUG
 #define debug(args...) log(LOG_DEBUG, args);
#else
 #define debug(...)
#endif

#define MAXDEPS 50

static char elf_magic[16] = {0x7f, 'E', 'L', 'F', 01, 01, 01, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static inline void* bin_read(int fd, size_t offset, size_t size, void* inbuf, task_t* task) {
	void* buf = inbuf ? inbuf : kmalloc(size);
	vfs_seek(task, fd, offset, VFS_SEEK_SET);
	size_t read = vfs_read(task, fd, buf, size);
	if(likely(read == size)) {
		return buf;
	}

	debug("elf: bin_read: Read size %#x at offset %#x fd %d smaller than expected %#x\n", read, offset, fd, size);
	if(!inbuf) {
		kfree(buf);
	}
	return NULL;
}

static int load_phead(task_t* task, int fd, elf_program_header_t* phead, bool is_main) {
	if(phead->flags & PF_X && phead->flags & PF_W) {
		log(LOG_WARN, "elf: Program header marked as both executable and writable - refusing.\n");
		return -1;
	}

	size_t phys_offset = 0;
	void* virt;
	if(is_main) {
		virt = ALIGN_DOWN(phead->vaddr, PAGE_SIZE);
		phys_offset = phead->vaddr - virt;
	}

	size_t size = ALIGN(phead->memsz + phys_offset, PAGE_SIZE);

	vmem_t vmem;
	if(unlikely(valloc(VA_KERNEL, &vmem, RDIV(size, PAGE_SIZE), NULL, VM_RW | VM_ZERO) != 0)) {
		return -1;
	}

	if(is_main) {
		if(virt + size > task->sbrk) {
			task->sbrk = virt + size;
		}
	} else {
		virt = vmem.phys;
	}

	if(unlikely(!bin_read(fd, phead->offset, phead->filesz, vmem.addr + phys_offset, task))) {
		pfree((uint32_t)phys / PAGE_SIZE, size / PAGE_SIZE);
		return -1;
	}

	int vmem_flags = VM_USER | VM_TFORK | VM_NOCOW | VM_FREE;
	if(phead->flags & PF_W) {
		vmem_flags |= VM_RW;
	}

	valloc_at(&task->vmem, NULL, RDIV(size, PAGE_SIZE), virt, vmem.phys, vmem_flags);

	debug("  phys %#-8x-%#-8x task virt %#-8x-%#-8x kernel virt %#-8x-%#-8x\n",
		vmem.phys, (uintptr_t)vmem.phys + size, virt, (uintptr_t)virt + size, vmem.addr, (uintptr_t)vmem.addr + size);
	return 0;
}

static int read_dyn_table(task_t* task, int fd, elf_program_header_t* phead) {
	void* dyn = bin_read(fd, phead->offset, phead->filesz, NULL, task);
	if(!dyn) {
		return -1;
	}

	elf_dyn_tag_t* tag = (elf_dyn_tag_t*)dyn;
	while((void*)(tag + 1) < dyn + phead->filesz && tag->tag) {
		switch(tag->tag) {
			#if 0
			case DT_NEEDED:
				if(task->elf_ctx.ndyndeps >= MAXDEPS) {
					return -1;
				}

				task->elf_ctx.dyndeps[task->elf_ctx.ndyndeps++] = tag->val;
				break;
			#endif
			case DT_STRTAB:
				task->elf_ctx.dynstrtab = (void*)tag->val;
				break;
		}
		tag++;
	}

	kfree(dyn);
	return 0;
}

static uint32_t read_pheads(task_t* task, int fd, elf_t* header, bool is_main) {
	elf_program_header_t* phead_start = bin_read(fd, header->phoff,
		header->phnum * header->phentsize, NULL, task);
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
				if(load_phead(task, fd, phead, is_main) < 0) {
					kfree(phead_start);
					return -1;
				}
				break;
			case PT_INTERP:
				if(is_main) {
					task->elf_ctx.interp = bin_read(fd, phead->offset, phead->filesz, NULL, task);
					kfree(phead_start);
					return task->elf_ctx.interp ? 0 : -1;
				}
				break;
			case PT_DYNAMIC:
				if(is_main) {
					if(read_dyn_table(task, fd, phead) < 0) {
						kfree(phead_start);
						return -1;
					}
				}
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
	vfs_close(task, fd);                       \
	kfree(header);                             \
	return -1;                                 \
}

static int load_file(task_t* task, char* path, bool is_main) {
	debug("elf: Loading %s\n", path);
	int fd = vfs_open(task, path, O_RDONLY);
	if(unlikely(fd < 0)) {
		return -1;
	}

	elf_t* header = bin_read(fd, 0, sizeof(elf_t), NULL, task);
	LF_ASSERT(header, "Could not read ELF header");
	LF_ASSERT(!memcmp(header->ident, elf_magic, sizeof(elf_magic)),
		"Invalid magic");

	if(is_main) {
		LF_ASSERT(header->type == ELF_TYPE_EXEC, "Binary is inexecutable");
	}

	LF_ASSERT(header->machine == ELF_ARCH_386, "Invalid architecture");
	LF_ASSERT(header->version == ELF_VERSION_CURRENT, "Unsupported ELF version");
	LF_ASSERT(header->entry, "Binary has no entry point");
	LF_ASSERT(header->phnum, "No program headers");
	LF_ASSERT(header->shnum, "No section headers");

	LF_ASSERT(read_pheads(task, fd, header, is_main) != -1, "Loading program headers failed");

	if(is_main) {
		task->entry = header->entry;
	}
	kfree(header);

	// setuid/setgid
	vfs_stat_t* stat = kmalloc(sizeof(vfs_stat_t));
	if(vfs_fstat(task, fd, stat) == 0) {
		if(stat->st_mode & S_ISUID) {
			task->euid = stat->st_uid;
		}
		if(stat->st_mode & S_ISGID) {
			task->egid = stat->st_gid;
		}
	}

	kfree(stat);
	vfs_close(task, fd);
	return 0;
}

int elf_load_file(task_t* task, char* path) {
	char* abs_path = vfs_normalize_path(path, task->cwd);
	strncpy(task->binary_path, abs_path, VFS_PATH_MAX);

	if(load_file(task, abs_path, true) < 0) {
		kfree(abs_path);
		sc_errno = ENOEXEC;
		return -1;
	}
	kfree(abs_path);

	if(task->elf_ctx.interp) {
		if(load_file(task, task->elf_ctx.interp, true) < 0) {
			sc_errno = ENOEXEC;
			return -1;
		}
	}

/*
	if(ctx->ndyndeps) {
		char* dynstr = (void*)vmem_translate(task->vmem_ctx, (intptr_t)ctx->dynstrtab, false);
		for(int i = 0; i < ctx->ndyndeps; i++) {
			char* lib_path = kmalloc(strlen(dynstr + ctx->dyndeps[i]) + 10);
			sprintf(lib_path, "/usr/lib/%s", dynstr + ctx->dyndeps[i]);

			if(load_file(task, lib_path, false) < 0) {
				kfree(lib_path);
				sc_errno = ENOEXEC;
				return -1;
			}
			kfree(lib_path);
		}
	}
*/
	task_set_initial_state(task);

	debug("elf: Entry point 0x%x, sbrk 0x%x\n", task->entry, task->sbrk);
	return 0;
}
