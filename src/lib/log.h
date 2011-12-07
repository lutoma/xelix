#pragma once

/* Copyright © 2010 Lukas Martini
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

#define LOG_INITSIZE 10000

// Logging levels
// LOG_NONE is only supposed to be used in setLogLevel().
#define LOG_ERR			1
#define LOG_WARN		2
#define LOG_INFO		3
#define LOG_DEBUG		4
#define LOG_ALWAYS		9001 // It's over 9000

void log(uint32_t level, const char *fmt, ...);
void log_setLogLevel(uint32_t level);
void log_setPrintLog(bool yesno);
void log_init();
