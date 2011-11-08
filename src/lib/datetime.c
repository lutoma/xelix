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

#include "string.h"
#include <hw/pit.h>
#include <memory/kmalloc.h>

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
	nowDate = readCMOS(whatDate);
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

// Calculate weekday
//See http://de.wikipedia.org/wiki/Wochentagsberechnung for details
int getWeekDay(int dayOfMonth, int month, int year)
{
	static int monthNums[] = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};

	int dayNum = dayOfMonth % 7;
	int monthNum = monthNums[month - 1];
	
	int yearNum = ((year / 10 % 10) * 10); // Get last two digits
	yearNum = (yearNum + yearNum / 4) % 7;

	int centuryNum = ((year /1000 % 1000) * 10); // Get first two digits
	centuryNum = (3 - (centuryNum % 4)) * 2;

	return (dayNum + monthNum + yearNum + centuryNum) % 7;
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
