/* memory.c: Memory initialization
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include "memory.h"

#include <lib/log.h>
#include "kmalloc.h"

extern bool __attribute__((__cdecl__)) a20_check(); // ASM.

void memory_init()
{
	init(kmalloc);
	
	bool a20 = a20_check();
	if(a20)
		log("memory: A20 line already enabled.\n");
	else // Todo: Enable it.
		log("memory: warning: A20 line is not enabled.\n");
}
