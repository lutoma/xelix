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
#include <lib/string.h>
#include <memory/kmalloc.h>
#include <hw/display.h>
#include <hw/keyboard.h>
#include <lib/datetime.h>
#include <filesystems/vfs.h>
#include <lib/fio.h>
#include <tasks/scheduler.h>

uint32_t cursorPosition;
char currentLine[256] = "";

// Print the command line prompt.
static void printPrompt()
{
	cursorPosition = 0;
	int colorold = display_getColor();
	display_setColor(0x07);
	print("\n> ");
	display_setColor(colorold);
}

// Execute a command
// Yes, this is only a bunch of hardcoded crap
static void executeCommand(char *command)
{
	if(strcmp(command, "reboot") == 0) reboot();
	else if(strcmp(command, "clear") == 0) clear();
	else if(strcmp(command, "ls") == 0)
	{
		struct dirent *node = 0;

		int i;
		for(i = 0; (node = vfs_rootNode->readDir(vfs_rootNode, i)); i++)
		{
			fsNode_t *fsNode = vfs_rootNode->findDir(vfs_rootNode, node->name);
			int color;
			if((fsNode->flags&0x7) == FS_DIRECTORY)
				color = 0x09;
			else
				color = 0x07;
			printf("%%%s%%  ", color, node->name);
		}
	}
	else if(strcmp(command, "cat") == 0)
	{
		fsNode_t *fsNode = vfs_rootNode->findDir(vfs_rootNode, "makememfs.c");
		
		uint8_t* buf = (uint8_t*)kmalloc(sizeof(char) * 100);
		int offset = 0;
		uint8_t sz = fsNode->read(fsNode, offset, 100, buf);

		while(sz != 0)
		{
			buf[sz] = 0; // Make sure it's NUL-terminated.
			printf("%s", buf);
			offset += 100;
			sz = fsNode->read(fsNode, offset, 100, buf);
		}
	}
	else if(strcmp(command, "pid") == 0)
	{
		task_t* proc = scheduler_getCurrentTask();
		if(proc != NULL)
			printf("procnum: %d\n", proc->pid);
	}
	else if(strcmp(command, "date") == 0)
	{
		int day = date('d');
		int month = date('M');
		int year = date('y');
		int hour = date('h');
		int minute = date('m');
		int second = date('s');
		int weekDay = getWeekDay(day, month, year);
		printf("%s %s %d %d:%d:%d UTC %d",dayToString(weekDay,1), monthToString(month,1), day, hour, minute, second, year);
	}
	else if(strcmp(command, "halt") == 0)
	{
		halt();
	} else
	{
		if(strlen(command) > 0 && command[0] != '-') // Note: I wanted / still want # for comments, however our keyboard driver doesn't know it...
			printf("error: command '%s' not found.\n", command);
	}
}

// Handle keyboard input.
static void handler(uint8_t code)
{
	char c = keyboard_codeToChar(code);

	if(c == NULL)
		return;
	
	if(c == 0x8) //0x8 is the backspace key.
	{
		if(cursorPosition < 1) return; // We don't want the user to remove the prompt ;)
		cursorPosition--;
		currentLine[strlen(currentLine) -1] = 0;
		char s[2] = { c, 0 };
		print(s);
		return;
	} else if(c == 0xA)
	{
		print("\n");
		executeCommand(currentLine);
		currentLine[0] = '\0';
		printPrompt();
		return;
	}	else cursorPosition++;

	if (strlen(currentLine) < sizeof(currentLine)-2) {
	    char s[2] = { c, 0 };
	    strcat(currentLine, s);
	    print(s);
        }
}

// Initialize the debug console.
void debugconsole_init()
{
	DUMPVAR("0x%x", currentLine);
	setLogLevel(0); // We don't want stuff to pop up in our console - use the kernellog command.
	keyboard_takeFocus(&handler);
	printPrompt();
}
