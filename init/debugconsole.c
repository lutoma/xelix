// A simple console for debugging purposes.
#include <init/debugconsole.h>

#ifdef WITH_DEBUGCONSOLE
#include <common/log.h>
#include <common/string.h>
#include <memory/kmalloc.h>
#include <devices/display/interface.h>
#include <devices/keyboard/interface.h>
#include <common/datetime.h>
#include <filesystems/vfs.h>

uint32 cursorPosition;
char currentLine[256] = "";
static char* currentDir = "/";

// Print the command line prompt.
static void printPrompt()
{
	cursorPosition = 0;
	int colorold = display_getColor();
	display_setColor(0x07);
	print(DEBUGCONSOLE_PROMPT);
	display_setColor(colorold);
}

// Execute a command
// Yes, this is only a bunch of hardcoded crap
static void executeCommand(char *command)
{
	if(strcmp(command, "reboot") == 0) reboot();
	else if(strcmp(command, "clear") == 0) clear();
	else if(strcmp(command, "help") == 0)
	{
		printf("%%You can use the following commands:%%\n", 0x04);
		printf("\treboot\n\tclear\n\tdate\n\tcolorinfo\n\thelp");
	}
	else if(strcmp(command, "ls") == 0)
	{
		int i = 0;
		struct dirent *node = 0;
		while ( (node = vfs_rootNode->readdir(vfs_rootNode, i) != 0) )
		{
			fsNode_t *fsNode = vfs_rootNode->finddir(vfs_rootNode, node->name);
			int color;
			if((fsNode->flags&0x7) == FS_DIRECTORY)
				color = 0x09;
			else
				color = 0x07;
			printf("%%%s%%  ", color, node->name);
			i++;
		}
	}
	else if(strcmp(command, "cat") == 0)
	{
		fsNode_t *fsNode = vfs_rootNode->finddir(vfs_rootNode, "makememfs.c");
		char buf[1000];
		fsNode->read(fsNode, 0, 1000, buf);
		printf("%s\n", buf);
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
	} else
	{
		if(strlen(command) > 0 && command[0] != '-') // Note: I wanted / still want # for comments, however our keyboard driver doesn't know it...
			printf("error: command '%s' not found.\n", command);
	}
}

// Handle keyboard input.
static void handler(char c)
{
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
	log("debugconsole: Initializing\n");
	DUMPVAR("0x%x", currentLine);
	setLogLevel(0); // We don't want stuff to pop up in our console - use the kernellog command.
	keyboard_takeFocus(&handler);
	printPrompt();
}
#endif
