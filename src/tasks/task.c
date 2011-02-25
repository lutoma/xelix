/* task.c: Some nice-to-have functions for easier task handling.
 * Copyright © 2010, 2011 Lukas Martini
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

#include "task.h"

#include "scheduler.h"
#include <memory/kmalloc.h>
#include <common/log.h>
#include <interrupts/interface.h>
#include <memory/paging/paging.h>

// Start process. The name parameter is here for future use.
void process_create(char name[100], void function())
{
	log("process: Spawned new process with name %s\n", name);
	(*function) (); // Run process
	return;
}
