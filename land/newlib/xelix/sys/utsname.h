/* Copyright © 2018 Lukas Martini
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

#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H
#ifdef __cplusplus
extern "C" {
#endif

struct utsname {
	char sysname[50];
	char nodename[300];
	char release[50];
	char version[300];
	char machine[50];
};

int uname(struct utsname *);

#ifdef __cplusplus
}       /* C++ */
#endif
#endif /* SYS_UTSNAME_H */
