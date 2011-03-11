/* generic.c: Support for builtin PC speakers
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

#include <lib/generic.h>
#ifdef WITH_SPEAKER
#include "speaker.h"

#include <lib/log.h>
#include <lib/datetime.h>

void speaker_on()
{
	outb(0x61,inb(0x61) | 3);
}

void speaker_off()
{
	outb(0x61,inb(0x61) &~3);
}

void speaker_setFrequency(uint8 frequency)
{
	uint8 divisor;
	divisor = 1193180L/frequency;
	outb(0x43,0xB6);
	outb(0x42,divisor&0xFF);
	outb(0x42,divisor >> 8);
}

void speaker_beep(uint8 frequency, time_t seconds)
{
	speaker_setFrequency(frequency);
	speaker_on();
	sleep(seconds);
	speaker_off();
}
#endif
