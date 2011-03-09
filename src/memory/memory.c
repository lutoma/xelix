/* memory.c: Memory initialization
 * Copyright © 2010 Christoph Sünderhauf
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

#include <common/log.h>
#include <memory/interface.h>
#include <memory/segmentation/gdt.h>
#include <common/generic.h>

void memory_init_preprotected()
{
	gdt_init();
	log("memory: Initialized preprotected memory\n");
}

void memory_init_postprotected()
{
	log("memory: Initialized postprotected memory\n");
}
