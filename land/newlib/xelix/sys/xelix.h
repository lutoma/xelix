/* Copyright © 2018-2021 Lukas Martini
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

#ifndef _SYS_XELIX_H
#define _SYS_XELIX_H

#include <stdint.h>
#include <errno.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _xelix_execdata {
	uint32_t pid;
	uint32_t ppid;
	uint32_t argc;
	uint32_t envc;
	char** argv;
	char** env;
	// binary_path in older versions of execdata
	char _unused[256];
	uint16_t uid;
	uint16_t gid;
	uint16_t euid;
	uint16_t egid;
	char binary_path[PATH_MAX];
};

extern struct _xelix_execdata* _xelix_execdata;
extern char* _progname;

int _strace(void);

#define syscall(call, a1, a2, a3) __syscall(__errno(), call, (uint32_t)a1, (uint32_t)a2, (uint32_t)a3)
static inline uint32_t __syscall(int* errp, uint32_t call, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
	register uint32_t _call asm("eax") = call;
	register uint32_t _arg1 asm("ebx") = arg1;
	register uint32_t _arg2 asm("ecx") = arg2;
	register uint32_t _arg3 asm("edx") = arg3;
	register uint32_t result asm("eax");
	register uint32_t sce asm("ebx");

	asm volatile(
		"int $0x80;"

		: "=r" (result), "=r" (sce)
		: "r" (_call), "r" (_arg1), "r" (_arg2), "r" (_arg3)
		: "memory");

	*errp = sce;
	return result;
}

#ifdef __cplusplus
}       /* C++ */
#endif
#endif /* _SYS_XELIX_H */
