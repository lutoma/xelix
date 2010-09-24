/** @file init/debugconsole.c
 * \brief A simple console for debugging purposes.
 * @author Lukas Martini
 */

#include <init/debugconsole.h>
#include <common/string.h>
#include <memory/kmalloc.h>
#include <devices/keyboard/interface.h>

uint32 cursorPosition;
char* currentLine;
void printPrompt();
void executeCommand(char* command);

/// Print the command line prompt.
void printPrompt()
{
	cursorPosition = 0;
	print("\n> ");
}

/** Execute a command
 * @param command The command to be executed
 */
void executeCommand(char* command)
{
	print("\n");
	print(command);
	print("\n");
}

/// Handle keyboard input.
void handler(char c)
{
	if(c == 0x8) //0x8 is the backspace key.
	{
		if(cursorPosition < 1) return; // We don't want the user to remove the prompt ;)
		cursorPosition--;
	} else if(c == 0xA)
	{
		executeCommand(currentLine);
		currentLine = "";
		printPrompt();
		return;
	}	else cursorPosition++;

	char s[2];
	s[0] = c;
	s[1] = 0;
	
	currentLine = strcat(currentLine, s);
	print(s);
}

/// Initialize the debug console.
void debugconsole_init()
{
	log("Initializing debug console\n");
	currentLine = kmalloc(600);
	setLogLevel(0); // We don't want stuff to pop up in our console - use the kernellog command.
	keyboard_takeFocus(&handler);
	printPrompt();
}
