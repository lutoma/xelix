#include <init/debugconsole.h>
#include <devices/keyboard/interface.h>

void printPrompt();
void printPrompt()
{
	print("\n> ");
}

void handler(char c)
{
			char s[2];
			s[0] = c;
			s[1] = 0;
			print(s);
}

void debugconsole_init()
{
	log("Initializing debug console\n");
	setLogLevel(0); // We don't want stuff to pop up in our console - use the kernellog command.
	keyboard_takeFocus(&handler);
}
