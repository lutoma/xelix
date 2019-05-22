#pragma once

/* Copyright © 2019 Lukas Martini
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

#include <stdbool.h>
#include <fs/vfs.h>

struct tty_input_state {
	bool shift;
	bool shift_left;
	bool shift_right;
	bool capslock;
	bool control_left;
	bool control_right;
	bool alt_left;
	bool alt_right;
	bool super;
	uint16_t code;
};

void tty_input_cb(struct tty_input_state* input);
int tty_poll(vfs_file_t* fp, int events);
