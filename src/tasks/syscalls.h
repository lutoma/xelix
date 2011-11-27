#pragma once

/* Copyright © 2011 Fritz Grimpen
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

#include <lib/generic.h>
#include <tasks/syscall.h>

#include "syscalls/chg_sys_conv.h"
#include "syscalls/write.h"
#include "syscalls/exit.h"
#include "syscalls/getpid.h"
#include "syscalls/getppid.h"
#include "syscalls/read.h"
#include "syscalls/brk.h"
#include "syscalls/mmap.h"
#include "syscalls/munmap.h"

syscall_t syscall_table[] = {
	sys_chg_sys_conv,	// 0
	sys_exit,			// 1
	sys_read,			// 2
	sys_write,			// 3
	sys_getpid,			// 4
	sys_brk,			// 5
	sys_getppid,		// 6
	sys_mmap,			// 7
	sys_munmap,			// 8
};

