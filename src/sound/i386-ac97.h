#pragma once

/* Copyright © 2015, 2016 Lukas Martini
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

#include <bsp/i386-pci.h>
#include <stdbool.h>

#define AC97_MAX_CARDS 10

struct ac97_card {
	pci_device_t *device;
	int nambar;
	int nabmbar;
	struct buf_desc* descs;
	uint16_t sample_rate;
	uint32_t last_buffer;
	uint32_t last_written_buffer;

	bool is_playing;
	int playing_buffer;
	int last_wr_buffer;
};

struct ac97_card ac97_cards[AC97_MAX_CARDS];

void ac97_play(char* file);
void ac97_init();
