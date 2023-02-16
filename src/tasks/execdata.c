/* task.c: Task execdata setup
 * Copyright © 2018-2021 Lukas Martini
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

#include <tasks/task.h>
#include <mem/kmalloc.h>
#include <mem/mem.h>
#include <string.h>

struct execdata {
	uint32_t pid;
	uint32_t ppid;
	uint32_t argc;
	uint32_t envc;
	void** argv;
	void** env;
	/* This used to be the binary_path when VFS_PATH_MAX was 256 bytes. To not
	 * break older binaries, keep it around for now.
	 */
	char old_binary_path[256];
	uint16_t uid;
	uint16_t gid;
	uint16_t euid;
	uint16_t egid;
	char binary_path[VFS_PATH_MAX];
};

/* Sets up four pages of runtime data for the program, including PID, argv,
 * environment etc.
 *
 * Page layout:
 * - struct execdata
 * - char** argv
 * - argv strings
 * - char** environ
 * - environ strings / free space for new environment variables
 */
void task_setup_execdata(task_t* task) {
	vmem_t vmem;
	valloc(VA_KERNEL, &vmem, 4, NULL, VM_RW | VM_ZERO);
	valloc_at(&task->vmem, NULL, 4, (void*)CONFIG_EXECDATA_LOCATION, vmem.phys,
		VM_USER | VM_RW | VM_FREE);

	size_t offset = 0;

	struct execdata* exc = (struct execdata*)vmem.addr;
	offset += sizeof(struct execdata);

	char** argv = (char**)(vmem.addr + offset);
	exc->argv = (void*)CONFIG_EXECDATA_LOCATION + offset;
	offset += sizeof(char*) * (task->argc + 1);

	for(int i = 0; i < task->argc; i++) {
		argv[i] = (void*)CONFIG_EXECDATA_LOCATION + offset;
		strncpy((char*)(vmem.addr + offset), task->argv[i], 200);
		offset += strlen(task->argv[i]) + 1;
	}

	char** environ = (char**)(vmem.addr + offset);
	exc->env = (void*)CONFIG_EXECDATA_LOCATION + offset;
	offset += sizeof(char*) * (task->envc + 1);

	for(int i = 0; i < task->envc; i++) {
		environ[i] = (void*)CONFIG_EXECDATA_LOCATION + offset;
		strncpy((char*)(vmem.addr + offset), task->environ[i], 200);
		offset += strlen(task->environ[i]) + 1;
	}

	exc->pid = task->pid;
	exc->uid = task->uid;
	exc->gid = task->gid;
	exc->euid = task->euid;
	exc->egid = task->egid;
	exc->ppid = task->parent ? task->parent->pid : 0;
	exc->argc = task->argc;
	exc->envc = task->envc;
	memcpy(exc->binary_path, task->binary_path, VFS_PATH_MAX);
	memcpy(exc->old_binary_path, task->binary_path, 256);
}
