/* sbrk.c: Implementation of the POSIX sbrk syscall
 * Copyright © 2013-2018 Lukas Martini
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

#include <tasks/syscall.h>
#include <memory/vmem.h>
#include <memory/kmalloc.h>
#include <tasks/scheduler.h>
#include <log.h>
#include <string.h>
#include <errno.h>

SYSCALL_HANDLER(sbrk) {
	size_t length = VMEM_ALIGN(syscall.params[1]);
	task_t* task = syscall.task;

	if(length < 0 || length > 0x500000) {
		sc_errno = ENOMEM;
		return -1;
	}

	if(!length) {
		return (intptr_t)task->sbrk;
	}

	void* phys_addr = zmalloc_a(length);
	if(!phys_addr) {
		sc_errno = EAGAIN;
		return -1;
	}

	// FIXME sbrk is not set properly in elf.c (?)
	void* virt_addr = task->sbrk;
	task->sbrk += length;

	task_add_mem(task, virt_addr, phys_addr, length, VMEM_SECTION_HEAP,
		TASK_MEM_FORK | TASK_MEM_FREE);

	return (intptr_t)virt_addr;
}
