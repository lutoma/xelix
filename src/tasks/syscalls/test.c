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
#include <lib/log.h>

SYSCALL_HANDLER(test)
{
	log(LOG_DEBUG, "syscall: test: Test syscall successfull\n");
	log(LOG_DEBUG, "Parameters: EAX=0x%x EBX=0x%x ECX=0x%x EDX=0x%x ESI=0x%x EDI=0x%x ESP=0x%x\n",\
	  syscall.state->eax,
	  syscall.state->ebx,
	  syscall.state->ecx,
	  syscall.state->edx,
	  syscall.state->esi,
	  syscall.state->edi,
	  syscall.state->esp
	);

	SYSCALL_RETURN(syscall.params[0] + syscall.params[1] + syscall.params[2]);
}

