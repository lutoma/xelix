/* time.c: Time tracking
 * Copyright © 2018 Lukas Martini
 *
 * unix timestamp conversion:
 * Copyright © 2010-2018 Oryx Embedded SARL
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <lib/portio.h>
#include <hw/pit.h>
#include "time.h"

#define CURRENT_YEAR        2018

// FIXME Should get this from ACPI
int century_register = 0x00;

time_t last_timestamp = 0;
uint64_t last_tick = 0;

static int in_progress() {
	portio_out8(0x70, 0x0A);
	return (portio_in8(0x71) & 0x80);
}

static uint8_t get_register(int reg) {
	portio_out8(0x70, reg);
	return portio_in8(0x71);
}

static time_t read_rtc() {
	uint8_t century = 0;
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint32_t year;

	uint8_t last_second;
	uint8_t last_minute;
	uint8_t last_hour;
	uint8_t last_day;
	uint8_t last_month;
	uint8_t last_year;
	uint8_t last_century;
	uint8_t registerB;

	/* This uses the "read registers until you get the same values twice in a
	 * row" technique to avoid getting dodgy/inconsistent values due to RTC
	 * updates.
	 */
	while(in_progress());
	second = get_register(0x00);
	minute = get_register(0x02);
	hour = get_register(0x04);
	day = get_register(0x07);
	month = get_register(0x08);
	year = get_register(0x09);

	if(century_register != 0) {
		century = get_register(century_register);
	}

	do {
		last_second = second;
		last_minute = minute;
		last_hour = hour;
		last_day = day;
		last_month = month;
		last_year = year;
		last_century = century;

	    while (in_progress());
	    second = get_register(0x00);
	    minute = get_register(0x02);
	    hour = get_register(0x04);
	    day = get_register(0x07);
	    month = get_register(0x08);
	    year = get_register(0x09);

	    if(century_register != 0) {
	    	century = get_register(century_register);
	    }
	} while((last_second != second) || (last_minute != minute) || (last_hour != hour) ||
		(last_day != day) || (last_month != month) || (last_year != year) ||
		(last_century != century) );

	registerB = get_register(0x0B);

	// Convert BCD to binary values if necessary
	if(!(registerB & 0x04)) {
		second = (second & 0x0F) + ((second / 16) * 10);
		minute = (minute & 0x0F) + ((minute / 16) * 10);
		hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
		day = (day & 0x0F) + ((day / 16) * 10);
		month = (month & 0x0F) + ((month / 16) * 10);
		year = (year & 0x0F) + ((year / 16) * 10);
		if(century_register != 0) {
			century = (century & 0x0F) + ((century / 16) * 10);
		}
	}

	// Convert 12 hour clock to 24 hour clock if necessary
	if (!(registerB & 0x02) && (hour & 0x80)) {
		hour = ((hour & 0x7F) + 12) % 24;
	}

	// Calculate the full (4-digit) year
	if(century_register != 0) {
		year += century * 100;
	} else {
		year += (CURRENT_YEAR / 100) * 100;
		if(year < CURRENT_YEAR) year += 100;
	}

	// Convert to unix timestamp
	//January and February are counted as months 13 and 14 of the previous year
	if(month <= 2) {
		month += 12;
		year -= 1;
	}

	//Convert years to days
	time_t t = (365 * year) + (year / 4) - (year / 100) + (year / 400);
	//Convert months to days
	t += (30 * month) + (3 * (month + 1) / 5) + day;
	//Unix time starts on January 1st, 1970
	t -= 719561;
	//Convert days to seconds
	t *= 86400;
	//Add hours, minutes and seconds
	t += (3600 * hour) + (60 * minute) + second;

	return t;
}


int time_get(struct timeval* tp) {
	uint64_t tick_now = pit_getTickNum();

	/* Can't use libgcc's 64 bit integer division functions right now.
	 * The offset shouldn't be larger than a uint32 anyway.
	 */
	uint32_t offset = (tick_now - last_tick);
	last_timestamp += offset / PIT_RATE;
	last_tick = tick_now;

	tp->tv_sec = last_timestamp;
	tp->tv_usec = 0;

	return 0;
}

void time_init() {
	last_timestamp = read_rtc();
	last_tick = pit_getTickNum();
}
