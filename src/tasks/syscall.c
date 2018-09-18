/* syscall.c: Syscall handling
 * Copyright © 2011-2018 Lukas Martini
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

#include "syscall.h"
#include <hw/interrupts.h>
#include <log.h>
#include <tasks/scheduler.h>
#include <print.h>
#include <panic.h>
#include <memory/vmem.h>

#include "syscalls.h"

/* Resolves a virtual address from within the virtual address space of the
 * current task to the corresponding physical address we can use.
 * If reverse is true, do the exact opposite (phys address -> task address).
 */
intptr_t task_resolve_address(intptr_t raddress, bool reverse)
{
	struct vmem_page* (*_get_pg)(struct vmem_context*, void*) = reverse ? vmem_get_page_phys : vmem_get_page_virt;
	task_t* task = scheduler_get_current();

	int diff = raddress % PAGE_SIZE;
	raddress -= diff;

	struct vmem_page* page = _get_pg(task->memory_context, (void*)raddress);

	if(!page)
		return (intptr_t)NULL;

	intptr_t v = reverse ? (intptr_t)page->virt_addr : (intptr_t)page->phys_addr;
	return v + diff;
}

static void int_handler(cpu_state_t* regs)
{
	task_t* task = scheduler_get_current();
	if(!task) {
		log(LOG_WARN, "syscall: Got interrupt, but there is no current task.\n");
		return;
	}

	task->task_state = TASK_STATE_SYSCALL;

	struct syscall syscall;
	syscall.num = regs->eax;
	syscall.params[0] = regs->ebx;
	syscall.params[1] = regs->ecx;
	syscall.params[2] = regs->edx;
	syscall.state = regs;
	syscall.task = task;

	syscall_t call = syscall_table[syscall.num];
	if (syscall.num >= sizeof(syscall_table) / sizeof(syscall_t) || call == NULL)
	{
		log(LOG_INFO, "syscall: Invalid syscall %d\n", syscall.num);
		syscall.num = -1;
		return;
	}

#ifdef SYSCALL_DEBUG
	task_t* cur = scheduler_get_current();
	log(LOG_DEBUG, "PID %d <%s>: %s(0x%x 0x%x 0x%x)\n",
		cur->pid, cur->name,
		syscall_name_table[syscall.num],
		regs->ebx,
		regs->ecx,
		regs->edx);
#endif

	task->syscall_errno = 0;
	regs->eax = call(syscall);
	regs->ebx = task->syscall_errno;

	// Only change state back if it hasn't alreay been modified
	if(task->task_state == TASK_STATE_SYSCALL) {
		task->task_state = TASK_STATE_RUNNING;
	}

#ifdef SYSCALL_DEBUG
	log(LOG_DEBUG, "Result: %d, errno: %d\n", regs->eax, regs->ebx);
#endif
}

void syscall_init() {
	interrupts_register(SYSCALL_INTERRUPT, int_handler);
}
