/* sys_hostname.c: Set/get hostname
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

#include "uname.h"
#include <tasks/syscall.h>
#include <net/net.h>
#include <lib/string.h>

struct utsname {
	char sysname[20];
	char nodename[64];
	char release[20];
	char version[20];
	char machine[20];
};

int sys_uname(struct syscall syscall)
{
	struct utsname* info = (struct utsname*)syscall.params[0];
	strcpy(info->sysname, "Xelix");
	strcpy(info->nodename, net_get_hostname(64));
	// TODO change this once we have sprintf
	strcpy(info->release, "derp");
	strcpy(info->version, "derp");
	strcpy(info->machine, "i386");
	return 0;
}

