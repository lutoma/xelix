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

#include "generic.h"
#include <int/int.h>
#include <bsp/timer.h>
#include <tasks/scheduler.h>

#define assert(b) if(!(b)) panic("Assertion \"" #b "\" failed.")
#define assert_nc(qry) { if(!(qry)) {														\
	log(LOG_ERR, "Assertion \"" #qry "\" in " __FILE__ ":%d failed.", __LINE__);		\
	return;																					\
}}

// lib/walk_stack.asm
extern int walk_stack(intptr_t* addresses, int naddr);

char* addr2name(intptr_t address);
void __attribute__((optimize("O0"))) panic(char* error, ...);
