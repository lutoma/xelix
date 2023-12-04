/* Copyright © 2023 Lukas Martini
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

#include <stdio.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/xelix.h>

#define LD_ASSERT(cond, msg) \
	if(__builtin_expect(!(cond), 0)) { \
		fprintf(stderr, "ld-xelix: " msg "\n"); \
		exit(1); \
	}

#define debug(args...) if(do_debug) { printf(args); _serial_printf(args); }

// ld-xelix.asm
extern void plt_trampoline(void);

struct elf_object;
struct dependency {
	struct elf_object* obj;
	LIST_ENTRY(dependency) entries;

};

struct elf_object {
	LIST_ENTRY(elf_object) entries;
	LIST_HEAD(elf_dep_head, dependency) deps;

	char* path;
	void* base_addr;

	int fd;
	int seek_offset;

	Elf32_Ehdr header;
	Elf32_Phdr* pheads;
	Elf32_Dyn* dyn;
	size_t dyn_count;

	Elf32_Sym* symbols;
	size_t syment;
	char* strtab;

	Elf32_Rel* rel;
	size_t relent;
	size_t relsz;
	uint32_t* pltgot;
	Elf32_Rel* jmprel;
	size_t pltrelsz;

	void* init_array;
	size_t init_array_size;

	void* fini_array;
	size_t fini_array_size;

	void (*init)(void);
	void (*fini)(void);
};

static bool do_debug = false;
static const char elf_magic[16] = {0x7f, 'E', 'L', 'F', 01, 01, 01, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static struct elf_object* root_obj = NULL;
LIST_HEAD(elf_objs_head, elf_object) loaded_objs;

static void load_obj(struct elf_object* _obj, char* path, struct elf_object* req_obj);

static void* read_data(struct elf_object* obj, void* buf, off_t offset, size_t size) {
	if(offset != obj->seek_offset) {
		if(lseek(obj->fd, offset, SEEK_SET) != offset) {
			perror("ld-xelix: Seek failed on executable file");
			exit(1);
		}

		obj->seek_offset = offset;
	}

	if(!buf) {
		buf = malloc(size);
	}

	if(read(obj->fd, buf, size) != size) {
		perror("ld-xelix: Reading executable data failed");
		exit(1);
	}

	obj->seek_offset += size;
	return buf;
}

static inline Elf32_Sym* find_sym(struct elf_object* obj, const char* name, bool allow_local) {
	Elf32_Sym* sym = obj->symbols;
	while(sym < obj->strtab) { // FIXME FIXME
		int bind = ELF32_ST_BIND(sym->st_info);
		if((allow_local || bind != STB_LOCAL) && sym->st_value &&
			!strcmp(obj->strtab + sym->st_name, name)) {

			return sym;
		}
		sym = (Elf32_Sym*)((uintptr_t)sym + obj->syment);
	}

	return NULL;
}

static inline void* resolve_dep_sym(struct elf_object* req_obj, const char* name) {
	void* result = NULL;

	// Check own syms
	Elf32_Sym* sym = find_sym(req_obj, name, true);
	if(sym) {
		int bind = ELF32_ST_BIND(sym->st_info);
		result = sym->st_value;
		if(req_obj->header.e_type == ET_DYN) {
			result += (uintptr_t)req_obj->base_addr;
		}

		// For non-weak symbols, return immediately.
		// For weak symbols, keep looking for potential non-weak sym.
		if(bind != STB_WEAK) {
			return result;
		}
	}

	for(struct dependency* np = req_obj->deps.lh_first; np; np = np->entries.le_next) {
		struct elf_object* obj = np->obj;
		if(!obj->symbols || !obj->strtab) {
			continue;
		}

		Elf32_Sym* sym = find_sym(obj, name, false);
		if(sym) {
			int bind = ELF32_ST_BIND(sym->st_info);
			if(bind == STB_LOCAL) {
				continue;
			}

			result = sym->st_value;
			if(obj->header.e_type == ET_DYN) {
				result += (uintptr_t)obj->base_addr;
			}

			if(bind != STB_WEAK) {
				return result;
			}
		}
	}

	// Check binary (unless we've already checked it above)
	if(req_obj != root_obj) {
		sym = find_sym(root_obj, name, true);
		if(sym) {
			if(ELF32_ST_BIND(sym->st_info) != STB_LOCAL) {
				result = sym->st_value;
				if(root_obj->header.e_type == ET_DYN) {
					result += (uintptr_t)root_obj->base_addr;
				}
			}
		}
	}

	return result;
}

void* __fastcall resolve_callback(uint32_t offset, struct elf_object* req_obj) {
	Elf32_Rel* rel = (Elf32_Rel*)((uintptr_t)req_obj->jmprel + offset);
	if(ELF32_R_TYPE(rel->r_info) != R_386_JMP_SLOT) {
		fprintf(stderr, "ld-xelix: Unsupported relocation type\n");
	}

 	int symidx = ELF32_R_SYM(rel->r_info);
	Elf32_Sym* sym = (Elf32_Sym*)((uintptr_t)req_obj->symbols + (symidx * req_obj->syment));
	const char* name = req_obj->strtab + sym->st_name;

	debug("ld-xelix: Lazy relocation %-35s in %s\n", name, req_obj->path);

	void* target = resolve_dep_sym(req_obj, name);
	if(!target) {
		fprintf(stderr, "ld-xelix: Could not resolve %s\n\n", name);
		exit(EXIT_FAILURE);
	}

	uintptr_t relptr = rel->r_offset;
	if(req_obj->header.e_type == ET_DYN) {
		relptr += req_obj->base_addr;
	}

	*(uint32_t*)relptr = target;
	return target;
}

static void relocate(struct elf_object* obj) {
	if(obj->rel && obj->relsz) {
		Elf32_Rel* crel = obj->rel;
		int count = 0;

		for(; (uintptr_t)crel < (uintptr_t)obj->rel + obj->relsz; crel++, count++) {
			int reltype = ELF32_R_TYPE(crel->r_info);
			uintptr_t relptr = crel->r_offset;
			if(obj->header.e_type == ET_DYN) {
				relptr += obj->base_addr;
			}

			// First handle relocations that don't need a symbol
			switch(reltype) {
			case R_386_RELATIVE:
				*(uint32_t*)relptr = *(uint32_t*)relptr + obj->base_addr;
				continue;
			}

			// Try loading symbol
			int symidx = ELF32_R_SYM(crel->r_info);
			if(!symidx) {
				fprintf(stderr, "ld-xelix: Could not handle symbol at %p\n", crel->r_offset);
				exit(EXIT_FAILURE);
			}

			Elf32_Sym* sym = (Elf32_Sym*)((uintptr_t)obj->symbols + (symidx * obj->syment));
			const char* name = obj->strtab + sym->st_name;
			void* target = resolve_dep_sym(obj, name);

			if(!target){
				if(ELF32_ST_BIND(sym->st_info) == STB_WEAK) {
					continue;
				}

				fprintf(stderr, "ld-xelix: Could not resolve required symbol %s\n", name);
				exit(EXIT_FAILURE);
			}

			switch(reltype) {
			case R_386_GLOB_DAT:
				*(uint32_t*)relptr = target;
				break;
			case R_386_32:
				*(uint32_t*)relptr += target;
				break;
			case R_386_PC32:
				// FIXME?
				*(uint32_t*)relptr += target;
				break;
			default:
				fprintf(stderr, "ld-xelix: Unsupported relocation type %d\n", reltype);
				exit(EXIT_FAILURE);
			}
		}

		debug("  Handled %d relocations\n", count);
	}

	if(obj->jmprel && obj->header.e_type == ET_DYN) {
		Elf32_Rel* crel = obj->jmprel;
		int count = 0;

		for(; (uintptr_t)crel < (uintptr_t)obj->jmprel + obj->pltrelsz; crel++, count++) {
			int reltype = ELF32_R_TYPE(crel->r_info);

			if(reltype == R_386_JMP_SLOT) {
				uint32_t* roffset = (uint32_t*)((uintptr_t)crel->r_offset + obj->base_addr);
				*roffset += obj->base_addr;
			} else {
				fprintf(stderr, "ld-xelix: Unsupported relocation type %d\n", reltype);
				exit(EXIT_FAILURE);
			}
		}

		debug("  Adjusted %d JMPREL entries\n", count);
	}
}

static void load_dyn(struct elf_object* obj) {
	// First, gather necessary context data from dynamic table
	Elf32_Dyn* dyn = obj->dyn;
	for(int i = 0; i < obj->dyn_count; i++) {
		uintptr_t relptr = dyn->d_un.d_ptr;

		if(relptr && obj->header.e_type == ET_DYN) {
			relptr += obj->base_addr;
		}

		switch(dyn->d_tag) {
		case DT_STRTAB:
			obj->strtab = (char*)relptr;
			break;
		case DT_SYMTAB:
			obj->symbols = (Elf32_Sym*)relptr;
			break;
		case DT_JMPREL:
			obj->jmprel = (Elf32_Rel*)relptr;
			break;
		case DT_PLTRELSZ:
			obj->pltrelsz = dyn->d_un.d_val;
			break;
		case DT_SYMENT:
			obj->syment = dyn->d_un.d_val;
			break;
		case DT_PLTGOT:
			obj->pltgot = (Elf32_Rel*)relptr;
			break;
		case DT_REL:
			obj->rel = (Elf32_Rel*)relptr;
			break;
		case DT_RELENT:
			obj->relent = dyn->d_un.d_val;
			break;
		case DT_RELSZ:
			obj->relsz = dyn->d_un.d_val;
			break;
		case DT_INIT:
			obj->init = relptr;
			break;
		case DT_FINI:
			obj->fini = relptr;
			break;
		case DT_INIT_ARRAY:
			obj->init_array = relptr;
			break;
		case DT_INIT_ARRAYSZ:
			obj->init_array_size = dyn->d_un.d_val;
			break;
		case DT_FINI_ARRAY:
			obj->fini_array = relptr;
			break;
		case DT_FINI_ARRAYSZ:
			obj->fini_array_size = dyn->d_un.d_val;
			break;
		}

		dyn++;
	}

	if(obj->pltgot) {
		obj->pltgot[1] = (uint32_t)obj;
		obj->pltgot[2] = (uint32_t)plt_trampoline;
	}


	if(do_debug) {
		dyn = obj->dyn;
		for(int i = 0; i < obj->dyn_count; i++) {
			if(dyn->d_tag != DT_NEEDED) {
				continue;
			}

			char* path;
			char* name = obj->strtab + dyn->d_un.d_val;
			if(asprintf(&path, "/usr/lib/%s", name) == -1) {
				fprintf(stderr, "ld-xelix: Could not build library path\n");
				exit(1);
			}

			debug("  Dependency: %s\n", name);
			free(path);
			dyn++;
		}
	}

	dyn = obj->dyn;
	for(int i = 0; i < obj->dyn_count; i++) {
		if(dyn->d_tag != DT_NEEDED) {
			continue;
		}

		char* path;
		char* name = obj->strtab + dyn->d_un.d_val;
		if(asprintf(&path, "/usr/lib/%s", name) == -1) {
			fprintf(stderr, "ld-xelix: Could not build library path\n");
			exit(1);
		}

		struct elf_object* dep = NULL;
		for(struct elf_object* cobj = loaded_objs.lh_first; cobj; cobj = cobj->entries.le_next) {
			if(!strcmp(cobj->path, path)) {
				dep = cobj;
			}
		}

		if(!dep) {
			dep = calloc(1, sizeof(struct elf_object));
			load_obj(dep, path, obj);
		}

		struct dependency* dl =  malloc(sizeof(struct dependency));
		dl->obj = dep;
		LIST_INSERT_HEAD(&obj->deps, dl, entries);
		dyn++;
	}
}

static void map_phead(struct elf_object* obj, Elf32_Phdr* phead) {
	LD_ASSERT(phead->p_filesz <= phead->p_memsz, "phead file size larger than memory size");

	void* addr_request = NULL;
	int mflags = MAP_ANONYMOUS | MAP_PRIVATE;

	// Position dependent
	if(obj->header.e_type != ET_DYN) {
		addr_request = (void*)phead->p_vaddr;
		mflags |= MAP_FIXED;
	} else if(obj->base_addr) {
		// PIC, but we already have allocated an earlier phead and now
		// need to allocate in relation to that one
		addr_request = (void*)((uintptr_t)obj->base_addr + phead->p_vaddr);
		mflags |= MAP_FIXED;
	} else {
		addr_request = 0x6000000;
	}

	// FIXME drop PROT_EXEC once mprotect stuff is ready
	void* addr = mmap(addr_request, phead->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, mflags, 0, 0);
	if(addr == MAP_FAILED || addr == -1) {
		fprintf(stderr, "ld-xelix: mmap failed at %p: %s\n", phead->p_vaddr, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if(!obj->base_addr) {
		obj->base_addr = addr;
	}

	// FIXME mmap directly
	read_data(obj, addr + (phead->p_vaddr % 0x1000), phead->p_offset, phead->p_filesz);

	int prot = 0;
	if(phead->p_flags & PF_R) {
		prot |= PROT_READ;
	}
	if(phead->p_flags & PF_X) {
		prot |= PROT_EXEC;
	}
	if(phead->p_flags & PF_W) {
		prot |= PROT_WRITE;
	}

	// FIXME
	// if(mprotect(addr, phead->p_memsz, prot) != 0) {
	// 	fprintf(stderr, "ld-xelix: Could not adjust protection of memory region at %p\n", addr);
	// 	exit(EXIT_FAILURE);
	// }

	debug("  map vaddr %-10p to %-10p from offset %#-6x size %#x\n",
		phead->p_vaddr, addr + (phead->p_vaddr % 0x1000),
		phead->p_offset, phead->p_memsz);
}

static void load_obj(struct elf_object* obj, char* path, struct elf_object* req_obj) {
	debug("\nOpening %s\n", path);

	LIST_INIT(&obj->deps);
	obj->path = path;
	obj->fd = open(path, O_RDONLY);
	if(obj->fd < 0) {
		fprintf(stderr, "ld-xelix: Could not open %s: %s\n", path, strerror(errno));
		free(obj);
		exit(EXIT_FAILURE);
	}

	Elf32_Ehdr* hdr = read_data(obj, &obj->header, 0, sizeof(Elf32_Ehdr));
	LD_ASSERT(memcmp(obj->header.e_ident, elf_magic, sizeof(elf_magic)) == 0, "Invalid ELF header magic");
	LD_ASSERT(obj->header.e_version == 1, "Binary has an unsupported ELF version");
	LD_ASSERT(obj->header.e_machine == EM_386, "Binary is for a different architecture");

	LD_ASSERT(obj->header.e_phnum > 0, "Binary has no program headers");
	LD_ASSERT(obj->header.e_type == ET_EXEC || obj->header.e_type == ET_DYN, "Binary is not an executable");

	if(!req_obj) {
		LD_ASSERT(obj->header.e_entry, "Binary has no entry point");
	}

	obj->seek_offset = 0;
	obj->base_addr = NULL;

	size_t pheads_size = obj->header.e_phnum * obj->header.e_phentsize;
	obj->pheads = read_data(obj, NULL, obj->header.e_phoff, pheads_size);

	Elf32_Phdr* phead = obj->pheads;
	phead = obj->pheads;
	for(int i = 0; i < obj->header.e_phnum; i++) {
		if(phead->p_type == PT_LOAD) {
			map_phead(obj, phead);
		}

		if(phead->p_type == PT_DYNAMIC) {
			obj->dyn = read_data(obj, NULL, phead->p_offset, phead->p_memsz);
			obj->dyn_count = phead->p_memsz / sizeof(Elf32_Dyn);
		}

		phead = (Elf32_Phdr*)((uintptr_t)phead + obj->header.e_phentsize);
	}

	// Now that other pheads are loaded, check dynamic section that relies on loaded data
	load_dyn(obj);
	close(obj->fd);
	LIST_INSERT_HEAD(&loaded_objs, obj, entries);
	return obj;
}

int main(int argc, char* argv[]) {
	setvbuf(stdout, NULL, _IONBF, 0);

	if(getenv("LD_DEBUG")) {
		do_debug = true;
	}

	char* execpath = _xelix_execdata->binary_path;
	if(!strcmp(execpath, "/usr/lib/ld-xelix.so")) {
		if(argc < 2) {
			fprintf(stderr, "Usage: ld-xelix.so <ELF object>\n(Or set as `interpreter` on ELF object).\n");
			exit(EXIT_FAILURE);
		}

		execpath = argv[1];
	}

	LD_ASSERT(execpath && execpath[0], "Could not get executable path.");
	root_obj = calloc(1, sizeof(struct elf_object));
	load_obj(root_obj, execpath, NULL);
	LD_ASSERT(root_obj, "Executable loading failed.");

	debug("\n");

	if(do_debug) {
		fflush(stdout);
		fflush(stderr);
	}

	struct elf_object* obj = root_obj;
	for(struct elf_object* obj = loaded_objs.lh_first; obj; obj = obj->entries.le_next) {
		debug("Relocating %s\n", obj->path);
		relocate(obj);
		debug("\n");
	}

		obj = root_obj;
	for(struct elf_object* obj = loaded_objs.lh_first; obj; obj = obj->entries.le_next) {
		if(obj->init) {
			debug("\nRunning init for %s at %p\n", obj->path, obj->init);
			obj->init();
		}
	}

	void (*entry)(void) = root_obj->header.e_entry;
	debug("\nStarting executable at %p\n", root_obj->header.e_entry);
	entry();

	for(struct elf_object* obj = loaded_objs.lh_first; obj; obj = obj->entries.le_next) {
		if(obj->fini) {
			debug("Running fini for %s at %p\n", obj->path, obj->fini);
			obj->fini();
		}
	}
}
