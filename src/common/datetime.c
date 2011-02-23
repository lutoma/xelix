/* datetime.c: Date / Time library
 * Copyright © 2010 Lukas Martini
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

#include "datetime.h"

#include <common/string.h>
#include <devices/pit/interface.h>

// Get the current day, month, year, time etc.
int date(char dateStr)
{
	int whatDate;
	int nowDate;
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
		default:
			return -1;
	}
	nowDate = inbCMOS(whatDate);
	switch(dateStr)
	{
		case 's':
		case 'm':
		case 'h':		
		case 'y':
			nowDate = (nowDate & 0xf) + 10 * (nowDate >> 4); // // time is returned in BCD format, translate to normal
	}
	if(dateStr == 'y') nowDate += 2000;
	return nowDate;
}

// Convert month number to month name
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
	
	monthString = longNames[month -1];
	if(shortVersion) monthString = substr(monthString, 0, 3);
	return monthString;
}

// Calculate weekday
//See http://de.wikipedia.org/wiki/Wochentagsberechnung for details
int getWeekDay(int dayOfMonth, int month, int year)
{
	int dayNum = dayOfMonth % 7;
	
	int monthNums[12];
	monthNums[0] = 0;
	monthNums[1] = 3;
	monthNums[2] = 3;
	monthNums[3] = 6;
	monthNums[4] = 1;
	monthNums[5] = 4;
	monthNums[6] = 6;
	monthNums[7] = 2;
	monthNums[8] = 5;
	monthNums[9] = 0;
	monthNums[10] = 3;
	monthNums[11] = 5;
	int monthNum = monthNums[month - 1];
	
	int yearNum = ((year / 10 % 10) * 10); // Get last two digits
	yearNum = (yearNum + yearNum / 4) % 7;

	int centuryNum = ((year /1000 % 1000) * 10); // Get first two digits
	centuryNum = (3 - (centuryNum % 4)) * 2;

	return (dayNum + monthNum + yearNum + centuryNum) % 7;
}

// Convert weekday number to weekday name
char* dayToString(int day, int shortVersion)
{
	char* dayString;
	char* longNames[7];
	
	longNames[0] = "Monday";
	longNames[1] = "Tuesday";
	longNames[2] = "Wednesday";
	longNames[3] = "Thursday";
	longNames[4] = "Friday";
	longNames[5] = "Saturday";
	longNames[6] = "Sunday";
	
	dayString = longNames[day-1];
	if(shortVersion)
		dayString = substr(dayString, 0, 3);
	return dayString;
}

// Sleep x seconds
void sleep(time_t timeout)
{
	timeout *= PIT_RATE;
	timeout++; // Make sure we always wait at least 'timeout'. One tick too long doesn't matter.
	int startTick = pit_getTickNum();
	while(1)
	{
		if(pit_getTickNum() > startTick + timeout) break;
	}
}
