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
	display_setColor(0x07);
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
		printf("%s %s %d %d:%d:%d UTC %d",dayToString(weekDay,1), monthToString(month,1), day, hour, minute, second, year);
	} 
	else if(strcmp(command, "color") == 0) 
	{
		//TODO Parameter: Farbe setzen
	} 
	else if(strcmp(command, "colorinfo") == 0) {
		printf("Black:\t\t0x00\n");
		printf("Blue:\t\t0x01\n");
		printf("Green:\t\t0x02\n");
		printf("Cyan:\t\t0x03\n");
		printf("Red:\t\t0x04\n");
		printf("Lila:\t\t0x05\n");
		printf("Brown:\t\t0x06\n");
		printf("LightGray:\t0x07\n");
		printf("Gray:\t\t0x08\n");
		printf("LightBlue:\t0x09\n");
		printf("LightGreen:\t0x0A\n");
		printf("LightCyan:\t0x0B\n");
		printf("Orange:\t\t0x0C\n");
		printf("Pink:\t\t0x0D\n");
		printf("Yellow:\t\t0x0E\n");
		printf("White:\t\t0x0F\n");
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
	log("Debuconsole currentLine position in memory: 0x%x\n", currentLine);
	setLogLevel(0); // We don't want stuff to pop up in our console - use the kernellog command.
	keyboard_takeFocus(&handler);
	printPrompt();
}
