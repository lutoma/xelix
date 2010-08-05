#include <init/debugconsole.h>
#include <devices/display/interface.h>

void handleInput(char* input);
int intCurPos = 0;
char* prompt = "\ndebug> ";
char* line = "";

void handleInput(char* input)
{
  if(strcmp("\b", input) == 0) return;
  //{
  //  if(intCurPos < 1) return;
  //  --intCurPos;
  //} else ++intCurPos;
  
  if(strcmp("\n", input) == 0)
  {
    if(strlen(line) > 0)
    {
      print("\n");
      print(line);
      print(": Command not found.");
    }
    strcpy(line, "");
    intCurPos = 0;
    print(prompt);
  } else {
    line = strcat(line, input);
    print(input);
  }
  
}

void debugconsole_init()
{
 log("\nStarted Decore debug shell");
 common_setLogLevel(0); // We don't want logging stuff to pop up in our debug console
 print(prompt);
 keyboard_takeFocus(&handleInput);
}
