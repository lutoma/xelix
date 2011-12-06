/* test.c: A syscall for testing purposes
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

#include "test.h"

#include "write.h"
#include <console/interface.h>
#include <lib/log.h>

int sys_test(struct syscall syscall)
{
	log(LOG_DEBUG, "syscall: test: Test syscall successfull\n");
	log(LOG_DEBUG, "Parameters: [0x%x][0x%x][0x%x][0x%x][0x%x][0x%x]\n",\
	  syscall.params[0],
	  syscall.params[1],
	  syscall.params[2],
	  syscall.params[3],
	  syscall.params[4],
	  syscall.params[5]
	);

	return syscall.params[0] + syscall.params[1] + syscall.params[1] +\
	  syscall.params[3] + syscall.params[4] + syscall.params[5];
}

