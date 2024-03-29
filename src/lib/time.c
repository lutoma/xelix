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

#include <portio.h>
#include <block/random.h>
#include <bsp/timer.h>
#include <fs/sysfs.h>
#include <tasks/task.h>
#include <log.h>
#include "time.h"

#define CURRENT_YEAR        2018

// FIXME Should get this from ACPI
int century_register = 0x00;

time_t last_timestamp = 0;
uint64_t last_tick = 0;

static int in_progress(void) {
	outb(0x70, 0x0A);
	return (inb(0x71) & 0x80);
}

static uint8_t get_register(int reg) {
	outb(0x70, reg);
	return inb(0x71);
}

static time_t read_rtc(void) {
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

uint32_t time_get(void) {
	uint32_t stick = timer_tick;
	if(stick <= last_tick + timer_rate) {
		return last_timestamp;
	}

	uint32_t offset = (stick - last_tick);
	last_timestamp += offset / timer_rate;
	last_tick = stick;
	return last_timestamp;
}

int time_get_timeval(task_t* task, struct timeval* tv) {
	tv->tv_sec = time_get();
	tv->tv_usec = 0;
	return 0;
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	size_t rsize = 0;
	sysfs_printf("%d", time_get());
	return rsize;
}

void time_init(void) {
	last_timestamp = read_rtc();
	last_tick = timer_tick;
	log(LOG_INFO, "time: Initial last_timestamp is %u at tick %llu\n", last_timestamp, last_tick);
	block_random_seed(last_timestamp + last_tick);

	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("time", &sfs_cb);
}
