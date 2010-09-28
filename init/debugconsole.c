// A simple console for debugging purposes.

#include <init/debugconsole.h>
#include <common/string.h>
#include <memory/kmalloc.h>
#include <devices/keyboard/interface.h>
#include <common/datetime.h>

uint32 cursorPosition;
char currentLine[256] = "";
void printPrompt();
void executeCommand(char *command);

// Print the command line prompt.
void printPrompt()
{
	cursorPosition = 0;
	print("\n> ");
}

// Execute a command
// Yes, this is only a bunch of hardcoded crap
void executeCommand(char *command)
{
	if(strcmp(command, "reboot") == 0) reboot();
	else if(strcmp(command, "clear") == 0) clear();
	else if(strcmp(command, "date") == 0)
	{
		int day = date('d');
		int month = date('M');
		int year = date('y');
		int hour = date('h');
		int minute = date('m');
		int second = date('s');
		int weekDay = getWeekDay(day, month, year);
		
		print(dayToString(weekDay,1));
		print(" ");
		print(monthToString(month,1));
		print(" ");
		printDec(day);
		print(" ");
		printDec(hour);
		print(":");
		printDec(minute);
		print(":");
		printDec(second);
		print(" UTC ");
		printDec(year);
	} else
	{
		print("error: command \"");
		print(command);
		print("\" not found");
	}
}

// Handle keyboard input.
void handler(char c)
{
	if(c == 0x8) //0x8 is the backspace key.
	{
		if(cursorPosition < 1) return; // We don't want the user to remove the prompt ;)
		cursorPosition--;
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
	log("Initializing debug console\n");
	log("Debuconsole currentLine position in memory: ");
	logHex(currentLine);
	log("\n");
	setLogLevel(0); // We don't want stuff to pop up in our console - use the kernellog command.
	keyboard_takeFocus(&handler);
	printPrompt();
}
