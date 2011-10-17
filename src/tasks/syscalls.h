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

syscall_t syscall_table[] = {
	sys_chg_sys_conv, // 0
	sys_exit, // 1
	NULL, // 2
	sys_read,
	sys_write, // 4
	NULL,
	NULL,
	NULL,
	NULL, // 8
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, // 16
	NULL,
	NULL,
	NULL,
	sys_getpid,
	NULL,
	NULL,
	NULL,
	NULL, // 24
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, // 32
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, // 40
	NULL,
	NULL,
	NULL,
	NULL,
	sys_brk, // 45
	NULL,
	NULL,
	NULL, // 48
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, // 56
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	sys_getppid, // 64
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, // 72
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, // 80
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, // 88
	NULL,
	sys_mmap, // 90
	NULL, // sys_munmap
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, // 96
	NULL,
	NULL,
};

