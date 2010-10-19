// A simple console for debugging purposes.
#include <init/debugconsole.h>

#ifdef WITH_DEBUGCONSOLE
#include <common/log.h>
#include <common/string.h>
#include <memory/kmalloc.h>
#include <devices/display/interface.h>
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
	int colorold = display_getColor();
	display_setColor(0x07);
	print(DEBUGCONSOLE_PROMPT);
	display_setColor(colorold);
}

// Execute a command
// Yes, this is only a bunch of hardcoded crap
void executeCommand(char *command)
{
	if(strcmp(command, "reboot") == 0) reboot();
	else if(strcmp(command, "clear") == 0) clear();
	else if(strcmp(command, "help") == 0)
	{
		printf("%%You can use the following commands:%%\n", 0x04);
		printf("\treboot\n\tclear\n\tdate\n\tcolorinfo\n\thelp");
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
	else if(strcmp(command, "colornext") == 0) 
	{
		
	} 
	else if(strcmp(command, "colorinfo") == 0) {
		printf("%% %% Black:\t\t0x00\n", 0x00);
		printf("%% %% Blue:\t\t\t0x01\n", 0x10);
		printf("%% %% Green:\t\t0x02\n", 0x20);
		printf("%% %% Cyan:\t\t\t0x03\n", 0x30);
		printf("%% %% Red:\t\t\t0x04\n", 0x40);
		printf("%% %% Purple:\t\t\t0x05\n", 0x50);
		printf("%% %% Brown:\t\t0x06\n", 0x60);
		printf("%% %% LightGray:\t0x07\n", 0x70);
		printf("%% %% Gray:\t\t\t0x08\n", 0x80);
		printf("%% %% LightBlue:\t0x09\n", 0x90);
		printf("%% %% LightGreen:\t0x0A\n", 0xA0);
		printf("%% %% LightCyan:\t0x0B\n", 0xB0);
		printf("%% %% Orange:\t\t0x0C\n", 0xC0);
		printf("%% %% Pink:\t\t\t0x0D\n", 0xD0);
		printf("%% %% Yellow:\t\t0x0E\n", 0xE0);;
		printf("%% %% White:\t\t0x0F\n", 0xF0);
	} else
	{
		printf("error: command '%s' not found.\n", command);
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
	log("Debuconsole currentLine position in memory: 0x%x\n", currentLine);
	setLogLevel(0); // We don't want stuff to pop up in our console - use the kernellog command.
	keyboard_takeFocus(&handler);
	printPrompt();
}
#endif
