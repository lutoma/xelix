/* test.c: A syscall for testing purposes
 * Copyright © 2011-2015 Lukas Martini
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

#include <tasks/syscall.h>
#include <console/console.h>
#include <hw/cpu.h>
#include <log.h>

SYSCALL_HANDLER(test)
{
	log(LOG_INFO, "syscall: test: Test syscall successful\n");
	dump_cpu_state(LOG_INFO, syscall.state);

	SYSCALL_RETURN(syscall.params[0] + syscall.params[1] + syscall.params[2]);
}

