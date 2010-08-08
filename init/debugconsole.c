#include <init/debugconsole.h>
#include <devices/display/interface.h>
#include <devices/keyboard/interface.h>
#include <memory/kmalloc.h>
#include <common/datetime.h>
#include <common/acpi.h>

void handleInput(char* input);
int intCurPos = 0;
char* prompt = "\ndebug> ";
char** line;

void handleInput(char* input)
{
  if(!strcmp(*"\b", *input))
  {
    if(intCurPos < 1) return;
    --intCurPos;
  } else ++intCurPos;
  
  if(!strcmp("\n", *input))
  {
    if(strlen(*line) > 0)
    {
      if(!strcmp(*line,"kernellog")) //testcommand
      {
        print(*kernellog);
      } else if(!strcmp(*line,"date"))
      {
        print("\n");
        int month = date('M');
        int day = date('d');
        int year = date('y');
        int weekDay = getWeekDay(day, month, year);
        print(dayToString(weekDay));
        print(" ");
        print(monthToString(month, 1));
        print(" ");
        display_printDec(day);
        print(" ");
        display_printDec(date('h'));
        print(":");
        display_printDec(date('m'));
        print(":");
        display_printDec(date('s'));
        print(" ");
        print("UTC");
        print(" ");
        display_printDec(year);
      } else if(strcmp(*line,"weekday") == 0)
      {
        print("\n");
        print(dayToString(getWeekDay(date('d'), date('M'), date('y')),0));
      } else if(strcmp(*line,"clear") == 0)
      {
        clear();

      } else if(strcmp(*line,"halt") == 0)
      {
        print("\n");
        common_setLogLevel(1);
        acpiPowerOff();
      } else if(strcmp(*line,"reboot") == 0)
      {
        reboot();
      } else {
        print("\n");
        print(line);
        print(": Command not found.");
      }
    }
    strcpy(*line, "");
    intCurPos = 0;
    display_setColor(0x0f);
    print(prompt);
    display_setColor(0x07);
  } else {
    if(!strcmp(input,"\b"))
    {
      *line = substr(line, 0, strlen(*line)-1);
    }
    else
      *line = strcat(*line, input);
    print(input);
  }
  
}

void debugconsole_init()
{
  log("\nStarted Decore debug shell\n");
  line = (char**)kmalloc(40);
  common_setLogLevel(0); // We don't want logging stuff to pop up in our debug console. if you want to see the log, use the "kernellog" debug command.
  display_setColor(0x0f);
  print(prompt);
  display_setColor(0x07);
  keyboard_takeFocus(&handleInput);
}
