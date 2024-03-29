/* timer.c: Interface to the programmable interrupt timer
 * Copyright © 2010-2019 Lukas Martini
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

#include "timer.h"
#include <int/int.h>
#include <fs/sysfs.h>
#include <tasks/task.h>
#include <portio.h>
#include <time.h>

static uint32_t tick = 0;
static uint32_t rate = 1;

// The timer callback. Gets called every time the PIT fires.
static void timer_callback(task_t* task, isf_t* state, int num) {
	tick++;
}

uint32_t timer_get_tick(void) {
	return tick;
}

uint32_t timer_get_rate(void) {
	return rate;
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	size_t rsize = 0;
	sysfs_printf("%d %d %d", uptime(), tick, rate);
	return rsize;
}

// Initialize the PIT
void timer_init(void) {
	// preemptability setting here also affects scheduler, so leave set to false
	int_register(IRQ(0), &timer_callback, false);
	rate = CONFIG_PIT_RATE;

	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency. Important to note is
	// that the divisor must be small enough to fit into 16-bits.
	uint32_t divisor = 1193180 / rate;
	// Send the command byte.
	outb(0x43, 0x36);

	// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
	uint8_t l = (uint8_t)(divisor & 0xFF);
	uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

	// Send the frequency divisor.
	outb(0x40, l);
	outb(0x40, h);

	log(LOG_DEBUG, "pit: Timer frequency %d\n", rate);
}

void timer_init2(void) {
	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("tick", &sfs_cb);
}
