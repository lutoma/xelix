#pragma once

/* Copyright © 2010 Lukas Martini, Fritz Grimpen
 * Copyright © 2011 Lukas Martini
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

#include "generic.h"

// Standard configuration - May be overwritten by user.

#pragma once

#define PIT_RATE 500
#define DEBUGCONSOLE_PROMPT "\n> "
#define LOG_SAVE
#define LOG_MAXSIZE 10000
#define MEMORY_MAX_KMEM 0xBFFFFF
#define BUGFIX_MAIL "lukas.martini@unionhost.de"
#define IRC_CHANNEL "#xelix at irc.libertirc.net"

#define WITH_SERIAL true
#define WITH_DEBUGCONSOLE true
#define WITH_SPEAKER true
#define WITH_MEMFS true
