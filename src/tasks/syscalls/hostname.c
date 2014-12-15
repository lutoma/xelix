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

#include <tasks/syscall.h>
#include <net/net.h>
#include <lib/log.h>

int sys_get_hostname(struct syscall syscall)
{
	char* name = net_get_hostname(syscall.params[1]);
	memcpy((char*)syscall.params[0], name, syscall.params[1]);
	return 0;
}

int sys_set_hostname(struct syscall syscall)
{
	net_set_hostname((char*)syscall.params[0], syscall.params[1]);
	return 0;
}
