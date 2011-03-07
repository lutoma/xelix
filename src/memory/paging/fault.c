/* fault.c: Description of what this file does
 * Copyright © 2010 Christoph Sünderhauf
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

#include "fault.h"

#include "paging.h"
#include <interrupts/interface.h>
#include <common/log.h>

static void handler(registers_t regs)
{
	// get the address
	uint32 faultAddress;
	asm volatile("mov %%cr2, %0" : "=r" (faultAddress));
	
	uint8 notPresent = ! (regs.err_code & 0x1); // pagefault because page was not present?
	uint8 write = regs.err_code & 0x2; // pagefault caused by write (if unset->read)
	uint8 usermode = regs.err_code & 0x4; // during usermode (if unset->kernelmode)
	uint8 reservedoverwritten = regs.err_code & 0x8; // reserved bits overwritten (if set -> reserved bits were overwritten causing this page fault
	uint8 instructionfetch = regs.err_code & 0x10; // pagefault during instruction set (if set -> during instruction fetch)
	
	log("paging: pagefault at 0x%x: ", faultAddress);
	if(notPresent) log("not present, ");
	if(write) log("write, ");
	if(!write) log("read, ");
	if(usermode) log("user-mode, ");
	if(!usermode) log("kernel-mode, ");
	if(reservedoverwritten) log("reserved bits overwritten, ");
	if(instructionfetch) log("during instruction fetch");
	
	log("\n");
}

void paging_registerFaultHandler()
{
	interrupt_registerHandler(14, &handler);
}
