#pragma once

/* Copyright © 2011 Lukas Martini
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

#include <lib/generic.h>

// Making ponies fly.
#define init(C, args...) \
	do \
	{ \
		log(LOG_INFO, "\e[36m" #C ": Initializing at " __FILE__ ":%d [" #C "_init(" #args ")] (plain)\n\e[0m", __LINE__); \
		C ## _init(args); \
		log(LOG_ALWAYS, "\e[36m" #C ": Initialized at " __FILE__ ":%d [" #C "_init(" #args ")] (plain)\n\e[0m", __LINE__); \
	} while(0);

bool init_haveGrub;
