/* debugconsole.c: A simple console for debugging purposes.
 * Copyright © 2010, 2011 Lukas Martini
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "debugconsole.h"

#include <lib/log.h>
#include <lib/print.h>
#include <lib/string.h>
#include <memory/kmalloc.h>
#include <hw/display.h>
#include <hw/keyboard.h>
#include <lib/datetime.h>
#include <fs/vfs.h>
#include <lib/fio.h>
#include <tasks/scheduler.h>
#include <console/interface.h>
#include <lib/panic.h>
#include <memory/paging.h>

uint32_t cursorPosition;
char currentLine[256] = "";

// Print the command line prompt.
static void printPrompt()
{
	cursorPosition = 0;
	print("\n> ");
}

// Execute a command
// Yes, this is only a bunch of hardcoded crap
static void executeCommand(char *command)
{
	if(strcmp(command, "reboot") == 0) reboot();
	else if(strcmp(command, "clear") == 0) printf("\e[H\e[2J");
	else if(strcmp(command, "pid") == 0)
	{
		task_t* proc = scheduler_getCurrentTask();
		if(proc != NULL)
			printf("procnum: %d\n", proc->pid);
	}
	else if(strcmp(command, "halt") == 0) halt();
	else if(strcmp(command, "freeze") == 0) freeze();
	else if(strcmp(command, "panic") == 0) panic("Test panic for debugging");
	else if(strcmp(command, "kill") == 0) asm("mov %eax, 1; int 0x80;");
	else if(strcmp(command, "triplefault") == 0)
	{
		struct vm_context *ctx = vm_new();
		paging_apply(ctx);
	}
	else if(strcmp(command, "pagefault") == 0)
	{
		char *ptr = (char*)0x00000000;
		*ptr = 0x00;
	}
	else if(strcmp(command, "reload") == 0)
	{
		paging_apply(vm_kernelContext);
	}
	else if(strcmp(command, "rebuild") == 0)
	{
		vm_set_cache(vm_kernelContext, NULL);
		paging_apply(vm_kernelContext);
	}
	else
	{
		if(strlen(command) > 0 && command[0] != '-') // Note: I wanted / still want # for comments, however our keyboard driver doesn't know it...
			printf("error: command '%s' not found.\n", command);
	}
}

void debugconsole_init()
{
	int read_offset = 0;
	int read;

	while (1)
	{
		printPrompt();

		read_offset = 0;
		while (read_offset < 255)
		{
			read = console_read(NULL, currentLine + read_offset, 1);

			if (currentLine[read_offset] == 0x8 || currentLine[read_offset] == 0x7f)
			{
				if (read_offset == 0) continue;
				currentLine[read_offset--] = 0;
				currentLine[read_offset] = 0;
				console_write2(NULL, "\x08");
				continue;
			}

			if (read > 0)
				console_write(NULL, currentLine + read_offset, 1);

			if (currentLine[read_offset] == '\n' || currentLine[read_offset] == '\r')
			{
				currentLine[read_offset] = '\0';
				break;
			}

			read_offset += read;
		}

		executeCommand(currentLine);
	}
}
