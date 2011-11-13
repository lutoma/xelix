#pragma once

/* Copyright © 2011 Lukas Martini
 *
 * This file is part of Xlibc.
 *
 * Xlibc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Xlibc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Xlibc. If not, see <http://www.gnu.org/licenses/>.
 */

// stdlib.h is supposed to include all the stddef.h stuff
#include <stddef.h>

// Return values
#define EXIT_FAILURE -1
#define EXIT_SUCCESS  0

// Should be an own function as of POSIX
#define _Exit _exit

// Disregards atexit()
void _exit(int status);
// Cares about atexit()
void exit(int status);
