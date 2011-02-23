/* syscall.c: Syscall handling
 * Copyright © 2010 Christopg Sünderhauf
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

#include <interrupts/interface.h>

// handles a syscall. The parameter gives the processor status for the systemcall
uint32 syscallHandler(registers_t regs)
{
	switch(regs.eax)
	{
		case 1:
			printf((char*) regs.ebx);
			return 0;
		case 2:
			printf("%d", regs.ebx);
			return 0;
		case 3:
			printf("%x", regs.ebx);
			return 0;
		default:
			printf("unknown syscall no. %d!\n", regs.eax);
			return 0;
	}
}
