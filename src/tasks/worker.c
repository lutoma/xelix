/* worker.c: kernel worker tasks
 * Copyright © 2023 Lukas Martini
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

#include "worker.h"
#include <mem/kmalloc.h>
#include <mem/vm.h>
#include <tasks/task.h>
#include <mem/i386-gdt.h>

worker_t* worker_new(char* name, void* entry) {
	worker_t* worker = kmalloc(sizeof(worker_t));
	worker->entry = entry;
	worker->stopped = false;
	strncpy(worker->name, name, VFS_NAME_MAX);

	worker->state = vm_alloc(VM_KERNEL, NULL, 1, NULL, VM_RW);
	if(!worker->state) {
		return NULL;
	}
	bzero(worker->state, sizeof(isf_t));

	worker->stack = vm_alloc(VM_KERNEL, NULL, KERNEL_STACK_PAGES, NULL, VM_RW);
	if(!worker->stack) {
		return NULL;
	}

	worker->state->ds = GDT_SEG_DATA_PL0;
	worker->state->cr3 = (uint32_t)vm_pagedir(VM_KERNEL);
	worker->state->ebp = 0;
	worker->state->esp = (void*)worker->stack + KERNEL_STACK_SIZE - sizeof(iret_t);

	// Pass worker as first fastcall argument
	worker->state->ecx = (uint32_t)worker;

	// Return stack for iret
	iret_t* iret = (iret_t*)worker->state->esp;
	iret->eip = worker->entry;
	iret->cs = GDT_SEG_CODE_PL0;
	iret->eflags = EFLAGS_IF;
	iret->user_esp = worker->state->esp;
	iret->ss = GDT_SEG_DATA_PL0;
	return worker;
}

int worker_stop(worker_t* worker) {
	worker->stopped = true;
	return 0;
}

int worker_exit(worker_t* worker) {
	worker_stop(worker);
	scheduler_yield();
	return -1;
}
