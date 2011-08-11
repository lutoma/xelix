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
#include <interrupts/interface.h>
#include <hw/pit.h>
#include <tasks/scheduler.h>

// Don't use this one, use the macro below.
void panic_raw(char *file, uint32_t line, const char *reason, ...);

#define panic(args...) panic_raw( __FILE__, __LINE__, args)
#define assert(b) do { if(!(b)) panic_raw(__FILE__, __LINE__, "Assertion \"" #b "\" failed."); } while(0)
