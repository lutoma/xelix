#include <common/datetime.h>

int date(char dateStr)
{
  int whatDate;
  switch(dateStr)
  {
    case 's':
      whatDate = 0;
      break;
    case 'm':
      whatDate = 0x02;
      break;
    case 'h':
      whatDate = 0x04;
      break;
    case 'd':
      whatDate = 0x07;
      break;
    case 'M':
      whatDate = 0x08;
      break;
    case 'y':
      whatDate = 0x09;
      break;
  }
  outb(0x70, whatDate);
  return inb(0x71);
}

char* monthToString(int month, int shortVersion)
{
  char* monthString;
  char* longNames[12];
  
  longNames[0] = "January";
  longNames[1] = "February";
  longNames[2] = "March";
  longNames[3] = "April";
  longNames[4] = "May";
  longNames[5] = "June";
  longNames[6] = "July";
  longNames[7] = "August";
  longNames[8] = "September";
  longNames[9] = "October";
  longNames[10] = "November";
  longNames[11] = "December";
  
  --month;
  monthString = longNames[month -1];
  if(shortVersion) monthString = substr(monthString, 0, 3);
  return monthString;
}
